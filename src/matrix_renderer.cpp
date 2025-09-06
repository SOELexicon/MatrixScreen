#include "matrix_renderer.h"
#include "mask_loader.h"
#include "logger.h"
#include <thread>
#include <cmath>

MatrixRenderer::MatrixRenderer() 
    : m_lastUpdate(std::chrono::high_resolution_clock::now()),
      m_lastFrameTime(std::chrono::high_resolution_clock::now()),
      m_performanceMetrics(std::make_unique<PerformanceMetrics>()),
      m_batchRenderer(std::make_unique<BatchRenderer>()),
      m_gridCellPool(std::make_unique<MemoryPool<GridCell>>(2000, 1000)), // Large pool for grid cells
      m_dirtyRectManager(std::make_unique<DirtyRectManager>()),
      m_characterEffects(std::make_unique<CharacterEffects>()) {
}

MatrixRenderer::~MatrixRenderer() {
    Shutdown();
}

bool MatrixRenderer::Initialize(HWND hwnd, const MatrixSettings& settings) {
    m_settings = settings;
    
    // Configure performance metrics
    if (m_performanceMetrics) {
        m_performanceMetrics->SetEnabled(settings.showPerformanceMetrics);
    }
    
    // Configure performance optimizations
    if (m_batchRenderer) {
        m_batchRenderer->SetEnabled(settings.enableBatchRendering);
        m_batchRenderer->Initialize(1000);
    }
    
    if (m_dirtyRectManager) {
        m_dirtyRectManager->SetEnabled(settings.enableDirtyRectangles);
        m_dirtyRectManager->Initialize(m_screenWidth, m_screenHeight, 64);
    }
    
    // Configure character effects
    if (m_characterEffects) {
        m_characterEffects->Initialize(settings);
    }
    
    // Set up frame rate limiting
    if (settings.enableFrameRateLimiting && settings.targetFrameRate > 0) {
        m_targetFrameDuration = std::chrono::duration<float, std::milli>(1000.0f / settings.targetFrameRate);
    }
    
    if (!InitializeDirect3D(hwnd)) return false;
    if (!InitializeDirect2D()) return false;
    if (!InitializeDirectWrite()) return false;
    
    InitializeColumns();
    
    if (!settings.maskImagePath.empty()) {
        LoadMask(settings.maskImagePath);
    }
    
    return true;
}

void MatrixRenderer::Shutdown() {
    m_columns.clear();
    m_densityMap.clear();
    m_sparseGrid.clear();
    m_activeCells.clear();
    m_activeCellSet.clear();
}

bool MatrixRenderer::InitializeDirect3D(HWND hwnd) {
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    m_screenWidth = clientRect.right - clientRect.left;
    m_screenHeight = clientRect.bottom - clientRect.top;
    
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = 2;
    swapChainDesc.BufferDesc.Width = m_screenWidth;
    swapChainDesc.BufferDesc.Height = m_screenHeight;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = hwnd;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    
    D3D_FEATURE_LEVEL featureLevel;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        nullptr, 0, D3D11_SDK_VERSION,
        &swapChainDesc, &m_swapChain,
        &m_device, &featureLevel, &m_deviceContext);
    
    if (FAILED(hr)) return false;
    
    // Create render target view
    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
    hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (FAILED(hr)) return false;
    
    hr = m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, &m_renderTargetView);
    if (FAILED(hr)) return false;
    
    m_deviceContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), nullptr);
    
    D3D11_VIEWPORT viewport = {};
    viewport.Width = static_cast<float>(m_screenWidth);
    viewport.Height = static_cast<float>(m_screenHeight);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    m_deviceContext->RSSetViewports(1, &viewport);
    
    return true;
}

bool MatrixRenderer::InitializeDirect2D() {
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, m_d2dFactory.GetAddressOf());
    if (FAILED(hr)) return false;
    
    Microsoft::WRL::ComPtr<IDXGISurface> dxgiBackBuffer;
    hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer));
    if (FAILED(hr)) return false;
    
    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED));
    
    hr = m_d2dFactory->CreateDxgiSurfaceRenderTarget(
        dxgiBackBuffer.Get(), &props, &m_d2dRenderTarget);
    if (FAILED(hr)) return false;
    
    // Create brushes
    Color matrixColor = GetMatrixColor();
    hr = m_d2dRenderTarget->CreateSolidColorBrush(
        matrixColor.ToD2D1(), &m_greenBrush);
    if (FAILED(hr)) return false;
    
    hr = m_d2dRenderTarget->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::White), &m_whiteBrush);
    if (FAILED(hr)) return false;
    
    hr = m_d2dRenderTarget->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::Black), &m_fadeBrush);
    if (FAILED(hr)) return false;
    
    return true;
}

bool MatrixRenderer::InitializeDirectWrite() {
    HRESULT hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(m_writeFactory),
        reinterpret_cast<IUnknown**>(m_writeFactory.GetAddressOf()));
    if (FAILED(hr)) return false;
    
    hr = m_writeFactory->CreateTextFormat(
        m_settings.fontName.c_str(),
        nullptr,
        m_settings.boldFont ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        m_settings.fontSize,
        L"",
        &m_textFormat);
    if (FAILED(hr)) return false;
    
    m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    m_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    
    // Initialize font cache for performance
    InitializeFontCache();
    
    return true;
}

void MatrixRenderer::InitializeColumns() {
    m_columns.clear();
    
    std::uniform_real_distribution<float> speedDist(m_settings.minSpeed, m_settings.maxSpeed);
    std::uniform_real_distribution<float> yDist(-200.0f, -50.0f);
    
    // Create columns based on density setting
    int columnWidth = static_cast<int>(m_settings.fontSize * 0.8f);
    int baseColumnCount = std::max(1, m_screenWidth / columnWidth);
    
    // Use density to control how many columns we create (0.1 to 3.0 = 10% to 300%)
    int columnCount = static_cast<int>(baseColumnCount * m_settings.density);
    
    for (int i = 0; i < columnCount; ++i) {
        MatrixColumn column;
        // Distribute columns across the screen, allowing overlap when density > 1
        column.x = static_cast<float>((i * m_screenWidth) / columnCount);
        column.y = yDist(g_rng);
        column.baseSpeed = speedDist(g_rng);
        column.currentSpeed = column.baseSpeed;
        column.baseFontSize = m_settings.fontSize;
        column.layer = 0;
        column.isActive = true;
        
        // Initialize with random starting position in character sequence
        if (!m_settings.useCustomWord && m_settings.sequentialCharacters) {
            column.customWordIndex = std::uniform_int_distribution<int>(0, static_cast<int>(MATRIX_CHARS.size()) - 1)(g_rng);
        } else {
            column.customWordIndex = 0;
        }
        
        m_columns.push_back(std::move(column));
    }
    
    // Initialize the grid
    InitializeGrid();
}

void MatrixRenderer::InitializeGrid() {
    // Create grid based on font size
    int cellWidth = static_cast<int>(m_settings.fontSize * 0.8f);
    int cellHeight = static_cast<int>(m_settings.fontSize * 0.9f);
    
    m_gridWidth = std::max(1, m_screenWidth / cellWidth);
    m_gridHeight = std::max(1, m_screenHeight / cellHeight);
    
    // Clear optimized sparse storage
    m_sparseGrid.clear();
    m_activeCells.clear();
    m_activeCellSet.clear();
    
    // Reserve space for typical active cell count (about 10% of full grid)
    size_t estimatedActiveCells = (m_gridWidth * m_gridHeight) / 10;
    m_activeCells.reserve(estimatedActiveCells);
    m_sparseGrid.reserve(estimatedActiveCells);
}

void MatrixRenderer::LoadMask(const std::wstring& imagePath) {
    MaskLoader loader;
    if (loader.LoadFromFile(imagePath)) {
        auto maskData = loader.GetBitmapData();
        
        // Create density map from actual bitmap pixels
        m_densityMap = loader.CreateDensityMap(m_screenWidth, m_screenHeight);
        
        // Create Direct2D bitmap
        D2D1_BITMAP_PROPERTIES bitmapProps = D2D1::BitmapProperties(
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE));
        
        D2D1_SIZE_U size = D2D1::SizeU(
            static_cast<UINT32>(maskData.width),
            static_cast<UINT32>(maskData.height));
        
        HRESULT hr = m_d2dRenderTarget->CreateBitmap(
            size, maskData.pixels.data(),
            static_cast<UINT32>(maskData.width * 4),
            &bitmapProps, &m_maskBitmap);
        
        if (SUCCEEDED(hr)) {
            CreateDensityMap();
        }
    }
}

void MatrixRenderer::CreateDensityMap() {
    // Use the MaskLoader to create a proper density map from the actual bitmap
    MaskLoader loader;
    
    if (!m_settings.maskImagePath.empty() && loader.LoadFromFile(m_settings.maskImagePath)) {
        // Use the loaded bitmap data to create density map
        m_densityMap = loader.CreateDensityMap(m_screenWidth, m_screenHeight);
        return;
    }
    
    // If no mask or loading failed, create uniform density
    m_densityMap.resize(m_screenWidth);
    for (auto& row : m_densityMap) {
        row.resize(m_screenHeight, m_settings.density);
    }
}

Color MatrixRenderer::GetMatrixColor() const {
    // Base matrix color with configurable hue
    // Debug: Log the hue value and resulting color
    static float lastLoggedHue = -1.0f;
    if (std::abs(m_settings.hue - lastLoggedHue) > 0.1f) {
        Color result = Color::FromHSV(m_settings.hue, 0.8f, 0.9f, 1.0f);
        LOG_DEBUG("Using hue: " + std::to_string(m_settings.hue) + 
                 " -> RGB(" + std::to_string(result.r) + "," + 
                 std::to_string(result.g) + "," + std::to_string(result.b) + ")");
        lastLoggedHue = m_settings.hue;
    }
    
    return Color::FromHSV(m_settings.hue, 0.8f, 0.9f, 1.0f);
}

float MatrixRenderer::GetMaskBrightness(int x, int y) const {
    // Use the existing density map which already samples the mask
    if (m_densityMap.empty()) return 0.1f; // Low brightness if no mask
    
    // Clamp coordinates
    int clampedX = std::max(0, std::min(x, static_cast<int>(m_densityMap.size()) - 1));
    int clampedY = 0;
    if (clampedX < static_cast<int>(m_densityMap.size()) && !m_densityMap[clampedX].empty()) {
        clampedY = std::max(0, std::min(y, static_cast<int>(m_densityMap[clampedX].size()) - 1));
    }
    
    // Get brightness from density map (assumes density correlates with brightness)
    if (clampedX < static_cast<int>(m_densityMap.size()) && 
        clampedY < static_cast<int>(m_densityMap[clampedX].size())) {
        
        // Convert density to brightness - invert because mask is typically white=bright
        float density = m_densityMap[clampedX][clampedY];
        
        // For now, assume density IS brightness
        // This should work if the mask setup is correct
        return density;
    }
    
    return 0.1f; // Default low brightness
}

Color MatrixRenderer::GetDepthColor(float depth, float alpha) const {
    // Create color based on depth (3D effect) and alpha using configurable hue
    Color baseColor = GetMatrixColor();
    
    // Special handling for head characters (very bright alpha)
    if (m_settings.whiteHeadCharacters && alpha > 0.95f) {
        // Head characters get white/bright tint but still respect hue
        Color headColor = GetMatrixColor();
        // Make it brighter and add white tint
        headColor.r = std::min(1.0f, headColor.r + 0.3f);
        headColor.g = std::min(1.0f, headColor.g + 0.3f);
        headColor.b = std::min(1.0f, headColor.b + 0.3f);
        headColor.a = alpha;
        return headColor;
    }
    
    // Apply 3D depth effect if enabled
    if (m_settings.enable3DEffect) {
        // Apply depth range scaling (higher depthRange = more dramatic effect)
        float scaledDepth = 0.5f + (depth - 0.5f) * (m_settings.depthRange / 5.0f);
        scaledDepth = std::max(0.0f, std::min(1.0f, scaledDepth));
        
        // Closer objects (higher depth) are brighter
        float brightness = 0.3f + (scaledDepth * 0.7f);
        baseColor.r *= brightness;
        baseColor.g *= brightness; 
        baseColor.b *= brightness;
    }
    
    // Apply alpha and return
    baseColor.a = alpha;
    return baseColor;
}

float MatrixRenderer::GetDensityAt(int x, int y) const {
    if (m_densityMap.empty() || x < 0 || y < 0 || 
        x >= static_cast<int>(m_densityMap.size()) || 
        y >= static_cast<int>(m_densityMap[0].size())) {
        return m_settings.density;
    }
    
    return m_densityMap[x][y];
}

void MatrixRenderer::Update(float deltaTime) {
    // Update character effects system
    if (m_characterEffects) {
        m_characterEffects->Update(deltaTime);
    }
    
    UpdateColumns(deltaTime);
}

void MatrixRenderer::UpdateColumns(float deltaTime) {
    std::uniform_int_distribution<int> charDist(0, static_cast<int>(MATRIX_CHARS.size()) - 1);
    
    // Get rain intensity multiplier for dynamic rain effects
    float rainIntensity = 1.0f;
    if (m_characterEffects) {
        rainIntensity = m_characterEffects->GetRainIntensityMultiplier();
    }
    
    for (auto& column : m_columns) {
        // Apply rain intensity variation to speed (reduced motion consideration)
        float speedMultiplier = rainIntensity;
        if (m_settings.enableMotionReduction) {
            speedMultiplier *= 0.7f; // Slower movement for reduced motion
        }
        
        // Move column head down
        column.y += column.currentSpeed * speedMultiplier * deltaTime * 60.0f;
        
        // Calculate grid position
        int gridX = static_cast<int>(column.x / (m_settings.fontSize * 0.8f));
        int gridY = static_cast<int>(column.y / (m_settings.fontSize * 0.9f));
        
        // Check if head is in valid grid bounds
        if (gridX >= 0 && gridX < m_gridWidth && gridY >= 0 && gridY < m_gridHeight) {
            GridCell& cell = GetCell(gridX, gridY);
            
            // Only create new character if cell is empty or very faded
            if (!cell.isActive || cell.alpha < 0.1f) {
                // Get depth for 3D effects
                float depth = 0.5f;
                if (m_settings.useMask && m_settings.enable3DEffect) {
                    depth = GetMaskBrightness(static_cast<int>(column.x), static_cast<int>(column.y));
                }
                
                // Always place character - trails should appear everywhere
                // Select character based on settings
                if (m_settings.useCustomWord && !m_settings.customWord.empty()) {
                    // Use custom word logic
                    if (m_settings.sequentialCharacters) {
                        cell.character = m_settings.customWord.substr(column.customWordIndex % static_cast<int>(m_settings.customWord.length()), 1);
                        column.customWordIndex = (column.customWordIndex + 1) % static_cast<int>(m_settings.customWord.length());
                    } else {
                        // Random character from custom word
                        int charIndex = std::uniform_int_distribution<int>(0, static_cast<int>(m_settings.customWord.length()) - 1)(g_rng);
                        cell.character = m_settings.customWord.substr(charIndex, 1);
                    }
                } else if (m_characterEffects) {
                    // Use enhanced character selection with variety and depth-based weighting
                    cell.character = m_characterEffects->SelectCharacter(depth, m_settings.enableCharacterVariety);
                } else {
                    // Fallback to basic character selection
                    cell.character = MATRIX_CHARS[charDist(g_rng)];
                }
                
                cell.alpha = 1.0f; // Start bright
                cell.fontSize = m_settings.fontSize * (0.7f + depth * 0.6f); // Depth-based size
                cell.depth = depth;
                cell.isActive = true;
                
                // Add to active tracking
                SetCellActive(gridX, gridY, cell);
            }
        }
        
        // Reset column when off screen
        if (column.y > m_screenHeight + 100) {
            column.y = std::uniform_real_distribution<float>(-200.0f, -50.0f)(g_rng);
            
            // Reset to random starting character for Japanese sequential mode
            if (!m_settings.useCustomWord && m_settings.sequentialCharacters) {
                column.customWordIndex = std::uniform_int_distribution<int>(0, static_cast<int>(MATRIX_CHARS.size()) - 1)(g_rng);
            }
        }
    }
    
    // Update grid cells
    UpdateGrid(deltaTime);
}

void MatrixRenderer::UpdateGrid(float deltaTime) {
    // Only update active cells for massive performance gain
    auto it = m_activeCells.begin();
    while (it != m_activeCells.end()) {
        auto [x, y] = *it;
        uint64_t key = PackCoords(x, y);
        
        auto cellIt = m_sparseGrid.find(key);
        if (cellIt == m_sparseGrid.end()) {
            // Cell no longer exists, remove from active list
            m_activeCellSet.erase(key);
            it = m_activeCells.erase(it);
            continue;
        }
        
        GridCell& cell = cellIt->second;
        
        // Update character effects
        if (m_characterEffects) {
            // Start morphing occasionally
            m_characterEffects->StartMorphing(cell, m_settings.morphFrequency * deltaTime);
            m_characterEffects->UpdateMorphing(cell, deltaTime);
            
            // Start glitches occasionally
            m_characterEffects->StartGlitch(cell, m_settings.glitchFrequency * deltaTime);
            m_characterEffects->UpdateGlitch(cell, deltaTime);
            
            // Update phosphor glow
            m_characterEffects->UpdateGlow(cell, deltaTime);
        }
        
        // Update last update time for effects
        cell.lastUpdateTime += deltaTime;
        
        // Fade the character over time (adjusted by motion reduction setting)
        float fadeRate = m_settings.fadeRate;
        if (m_settings.enableMotionReduction) {
            fadeRate *= 0.5f; // Slower fading for reduced motion
        }
        cell.alpha -= fadeRate * deltaTime;
        
        // Deactivate when fully faded
        if (cell.alpha <= 0.0f) {
            cell.alpha = 0.0f;
            cell.isActive = false;
            cell.character = L"";
            
            // Remove from active tracking
            m_activeCellSet.erase(key);
            it = m_activeCells.erase(it);
            
            // Remove from sparse grid to save memory
            m_sparseGrid.erase(cellIt);
        } else {
            ++it;
        }
    }
}


void MatrixRenderer::Render() {
    // Start performance tracking
    if (m_performanceMetrics) {
        m_performanceMetrics->StartFrame();
    }
    
    // Frame rate limiting
    if (m_settings.enableFrameRateLimiting && m_settings.targetFrameRate > 0) {
        auto now = std::chrono::high_resolution_clock::now();
        auto timeSinceLastFrame = std::chrono::duration<float, std::milli>(now - m_lastFrameTime);
        
        if (timeSinceLastFrame < m_targetFrameDuration) {
            // Sleep to maintain target frame rate
            auto sleepTime = m_targetFrameDuration - timeSinceLastFrame;
            std::this_thread::sleep_for(sleepTime);
        }
        
        m_lastFrameTime = std::chrono::high_resolution_clock::now();
    }
    
    // Clear background
    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_deviceContext->ClearRenderTargetView(m_renderTargetView.Get(), clearColor);
    
    m_d2dRenderTarget->BeginDraw();
    m_d2dRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black));
    
    // Render mask as lighter background if available and enabled
    if (m_maskBitmap && m_settings.useMask && m_settings.showMaskBackground) {
        RenderMaskBackground();
    }
    
    // Use optimized rendering if enabled
    if (m_settings.enableBatchRendering || m_settings.enableDirtyRectangles) {
        RenderOptimized();
    } else {
        // Standard rendering
        RenderGrid();
        RenderColumns();
    }
    
    // Render performance metrics overlay
    if (m_performanceMetrics && m_settings.showPerformanceMetrics) {
        m_performanceMetrics->Render(m_d2dRenderTarget.Get(), m_writeFactory.Get());
    }
    
    HRESULT hr = m_d2dRenderTarget->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
        // Handle device lost scenario
        InitializeDirect2D();
    }
    
    // Use adaptive VSync if enabled
    UINT syncInterval = 1;
    if (m_settings.enableAdaptiveVSync) {
        // Adaptive VSync - tear if running behind
        syncInterval = 0;
    }
    
    m_swapChain->Present(syncInterval, 0);
    
    // End performance tracking
    if (m_performanceMetrics) {
        m_performanceMetrics->EndFrame();
    }
}

void MatrixRenderer::RenderGrid() {
    // Only render active cells - massive performance improvement!
    for (const auto& [x, y] : m_activeCells) {
        uint64_t key = PackCoords(x, y);
        auto it = m_sparseGrid.find(key);
        if (it == m_sparseGrid.end()) continue;
        
        const GridCell& cell = it->second;
        
        if (!cell.isActive || cell.alpha < 0.05f || cell.character.empty()) {
            continue; // Skip inactive or transparent cells
        }
        
        // Calculate screen position
        float screenX = static_cast<float>(x) * m_settings.fontSize * 0.8f;
        float screenY = static_cast<float>(y) * m_settings.fontSize * 0.9f;
        
        if (screenX < -50 || screenX > m_screenWidth + 50 || 
            screenY < -50 || screenY > m_screenHeight + 50) {
            continue; // Skip off-screen cells
        }
        
        // Get color based on depth and alpha
        Color color = GetDepthColor(cell.depth, cell.alpha);
        m_fadeBrush->SetColor(color.ToD2D1());
        
        // Create layout rect
        D2D1_RECT_F layoutRect = D2D1::RectF(
            screenX - cell.fontSize * 0.5f, screenY,
            screenX + cell.fontSize * 0.5f, screenY + cell.fontSize);
        
        // Use cached font format for performance
        IDWriteTextFormat* format = GetCachedFormat(cell.fontSize);
        if (format) {
            m_d2dRenderTarget->DrawText(
                cell.character.c_str(),
                static_cast<UINT32>(cell.character.length()),
                format,
                layoutRect,
                m_fadeBrush.Get());
        } else {
            // Fallback to default format if cache miss
            m_d2dRenderTarget->DrawText(
                cell.character.c_str(),
                static_cast<UINT32>(cell.character.length()),
                m_textFormat.Get(),
                layoutRect,
                m_fadeBrush.Get());
        }
    }
}

void MatrixRenderer::RenderColumns() {
    // Render column heads as bright white characters
    std::uniform_int_distribution<int> charDist(0, static_cast<int>(MATRIX_CHARS.size()) - 1);
    
    for (const auto& column : m_columns) {
        // Skip if off screen
        if (column.y < -50 || column.y > m_screenHeight + 50) {
            continue;
        }
        
        // Get character for head (always random when not using custom word)
        std::wstring headChar;
        if (m_settings.useCustomWord && !m_settings.customWord.empty()) {
            if (m_settings.sequentialCharacters) {
                headChar = m_settings.customWord.substr(column.customWordIndex % static_cast<int>(m_settings.customWord.length()), 1);
            } else {
                int charIndex = std::uniform_int_distribution<int>(0, static_cast<int>(m_settings.customWord.length()) - 1)(g_rng);
                headChar = m_settings.customWord.substr(charIndex, 1);
            }
        } else {
            // Always random Japanese character for heads
            headChar = MATRIX_CHARS[charDist(g_rng)];
        }
        
        // Set color for head based on settings and current hue
        if (m_settings.whiteHeadCharacters) {
            Color headColor = GetMatrixColor();
            // Make it brighter and add white tint
            headColor.r = std::min(1.0f, headColor.r + 0.4f);
            headColor.g = std::min(1.0f, headColor.g + 0.4f);
            headColor.b = std::min(1.0f, headColor.b + 0.4f);
            m_whiteBrush->SetColor(headColor.ToD2D1());
        } else {
            // Use bright version of current hue for heads
            Color headColor = GetMatrixColor();
            headColor.r = std::min(1.0f, headColor.r * 1.2f);
            headColor.g = std::min(1.0f, headColor.g * 1.2f);
            headColor.b = std::min(1.0f, headColor.b * 1.2f);
            m_whiteBrush->SetColor(headColor.ToD2D1());
        }
        
        // Create layout rect
        D2D1_RECT_F layoutRect = D2D1::RectF(
            column.x - column.baseFontSize * 0.5f, column.y,
            column.x + column.baseFontSize * 0.5f, column.y + column.baseFontSize);
        
        // Render the head character
        m_d2dRenderTarget->DrawText(
            headChar.c_str(),
            static_cast<UINT32>(headChar.length()),
            m_textFormat.Get(),
            layoutRect,
            m_whiteBrush.Get());
    }
}

void MatrixRenderer::RenderOptimized() {
    // Reset batch renderer for new frame
    if (m_batchRenderer && m_settings.enableBatchRendering) {
        m_batchRenderer->Reset();
    }
    
    // Update dirty rectangles if needed
    if (m_dirtyRectManager && m_settings.enableDirtyRectangles) {
        // Mark areas where columns are as dirty
        for (const auto& column : m_columns) {
            if (column.y >= -50 && column.y <= m_screenHeight + 50) {
                D2D1_RECT_F columnRect = D2D1::RectF(
                    column.x - column.baseFontSize,
                    column.y - column.baseFontSize,
                    column.x + column.baseFontSize,
                    column.y + column.baseFontSize);
                m_dirtyRectManager->MarkDirty(columnRect);
            }
        }
    }
    
    // Render grid cells (using batch renderer if enabled)
    size_t cellsRendered = 0;
    for (const auto& [x, y] : m_activeCells) {
        uint64_t key = PackCoords(x, y);
        auto it = m_sparseGrid.find(key);
        if (it == m_sparseGrid.end()) continue;
        
        const GridCell& cell = it->second;
        
        if (!cell.isActive || cell.alpha < 0.05f || cell.character.empty()) {
            continue;
        }
        
        // Calculate screen position
        float screenX = static_cast<float>(x) * m_settings.fontSize * 0.8f;
        float screenY = static_cast<float>(y) * m_settings.fontSize * 0.9f;
        
        if (screenX < -50 || screenX > m_screenWidth + 50 || 
            screenY < -50 || screenY > m_screenHeight + 50) {
            continue;
        }
        
        // Check if this cell is in a dirty region (if dirty rect optimization is enabled)
        D2D1_RECT_F cellRect = D2D1::RectF(
            screenX - cell.fontSize * 0.5f, screenY,
            screenX + cell.fontSize * 0.5f, screenY + cell.fontSize);
            
        if (m_dirtyRectManager && m_settings.enableDirtyRectangles) {
            if (!m_dirtyRectManager->IsRectDirty(cellRect)) {
                continue; // Skip non-dirty cells
            }
        }
        
        // Get the final character to display (considering morphing and glitching)
        std::wstring displayChar = cell.character;
        if (m_characterEffects) {
            displayChar = m_characterEffects->GetGlitchedCharacter(cell);
        }
        
        // Get color based on depth and alpha
        Color color = GetDepthColor(cell.depth, cell.alpha);
        
        // Add system disruption effects
        if (m_characterEffects && m_characterEffects->IsSystemDisrupted()) {
            float disruptionIntensity = m_characterEffects->GetSystemDisruptionIntensity();
            // Flicker effect during system disruption
            if (static_cast<int>(cell.lastUpdateTime * 30.0f * disruptionIntensity) % 3 == 0) {
                color.a *= 0.3f; // Make characters flicker
            }
            // Add slight red tint during disruption
            color.r += disruptionIntensity * 0.2f;
        }
        
        if (m_batchRenderer && m_settings.enableBatchRendering) {
            // Add to batch renderer
            m_batchRenderer->AddCharacter(displayChar, cellRect, color.ToD2D1(), cell.fontSize);
        } else {
            // Immediate rendering with glow effect
            if (m_settings.enablePhosphorGlow && cell.glowIntensity > 0.0f) {
                // Render glow first (slightly larger and more transparent)
                Color glowColor = m_characterEffects ? m_characterEffects->GetGlowColor(cell) : GetMatrixColor();
                glowColor.a *= 0.5f;
                
                D2D1_RECT_F glowRect = D2D1::RectF(
                    cellRect.left - 2, cellRect.top - 2,
                    cellRect.right + 2, cellRect.bottom + 2);
                    
                m_fadeBrush->SetColor(glowColor.ToD2D1());
                IDWriteTextFormat* format = GetCachedFormat(cell.fontSize * 1.1f);
                if (format) {
                    m_d2dRenderTarget->DrawText(
                        displayChar.c_str(),
                        static_cast<UINT32>(displayChar.length()),
                        format,
                        glowRect,
                        m_fadeBrush.Get());
                }
            }
            
            // Render main character
            m_fadeBrush->SetColor(color.ToD2D1());
            IDWriteTextFormat* format = GetCachedFormat(cell.fontSize);
            if (format) {
                m_d2dRenderTarget->DrawText(
                    displayChar.c_str(),
                    static_cast<UINT32>(displayChar.length()),
                    format,
                    cellRect,
                    m_fadeBrush.Get());
            }
        }
        
        cellsRendered++;
    }
    
    // Flush batch renderer
    if (m_batchRenderer && m_settings.enableBatchRendering) {
        m_batchRenderer->Flush(m_d2dRenderTarget.Get(), m_writeFactory.Get(), m_textFormat.Get());
    }
    
    // Render columns (always immediate rendering for heads)
    RenderColumns();
    
    // Clear dirty flags for next frame
    if (m_dirtyRectManager && m_settings.enableDirtyRectangles) {
        m_dirtyRectManager->ClearDirtyFlags();
    }
    
    // Log performance info (optional)
    if (m_settings.enableLogging && cellsRendered > 0) {
        Logger::Instance().Debug("Rendered " + std::to_string(cellsRendered) + " cells using optimized path");
    }
}

void MatrixRenderer::RenderMaskBackground() {
    if (!m_maskBitmap) return;
    
    // Create a lighter black brush for the mask background
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> maskBrush;
    HRESULT hr = m_d2dRenderTarget->CreateSolidColorBrush(
        D2D1::ColorF(0.1f, 0.1f, 0.1f, 0.8f), // Dark gray with transparency
        &maskBrush);
    
    if (FAILED(hr)) return;
    
    // Get mask bitmap size
    D2D1_SIZE_F maskSize = m_maskBitmap->GetSize();
    
    // Calculate destination rectangle to fit screen
    D2D1_RECT_F destRect = D2D1::RectF(0, 0, 
        static_cast<float>(m_screenWidth), 
        static_cast<float>(m_screenHeight));
    
    // Draw the mask bitmap with configurable opacity
    m_d2dRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
    m_d2dRenderTarget->DrawBitmap(
        m_maskBitmap.Get(),
        &destRect,
        m_settings.maskBackgroundOpacity, // Configurable opacity
        D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
        nullptr);
}

void MatrixRenderer::Resize(int width, int height) {
    if (m_swapChain && (width != m_screenWidth || height != m_screenHeight)) {
        m_screenWidth = width;
        m_screenHeight = height;
        
        m_d2dRenderTarget.Reset();
        m_renderTargetView.Reset();
        
        HRESULT hr = m_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
        if (SUCCEEDED(hr)) {
            InitializeDirect2D();
            InitializeColumns();
            if (m_maskBitmap) {
                CreateDensityMap();
            }
        }
    }
}

void MatrixRenderer::UpdateSettings(const MatrixSettings& settings) {
    m_settings = settings;
    
    // Update performance metrics
    if (m_performanceMetrics) {
        m_performanceMetrics->SetEnabled(settings.showPerformanceMetrics);
    }
    
    // Update frame rate limiting
    if (settings.enableFrameRateLimiting && settings.targetFrameRate > 0) {
        m_targetFrameDuration = std::chrono::duration<float, std::milli>(1000.0f / settings.targetFrameRate);
    }
    
    // Recreate text format if font changed
    if (m_textFormat) {
        m_textFormat.Reset();
        InitializeDirectWrite();
    }
    
    // Update brush colors
    if (m_greenBrush) {
        Color matrixColor = GetMatrixColor();
        m_greenBrush->SetColor(matrixColor.ToD2D1());
    }
    
    // Update other brushes to maintain color consistency
    if (m_whiteBrush) {
        Color headColor = GetMatrixColor();
        headColor.r = std::min(1.0f, headColor.r + 0.3f);
        headColor.g = std::min(1.0f, headColor.g + 0.3f);
        headColor.b = std::min(1.0f, headColor.b + 0.3f);
        m_whiteBrush->SetColor(headColor.ToD2D1());
    }
    
    // Update character effects settings
    if (m_characterEffects) {
        m_characterEffects->SetSettings(settings);
    }
    
    InitializeColumns();
}

// Optimization helper implementations
void MatrixRenderer::InitializeFontCache() {
    // Pre-create font formats for common sizes
    for (size_t i = 0; i < FONT_CACHE_SIZE; ++i) {
        HRESULT hr = m_writeFactory->CreateTextFormat(
            m_settings.fontName.c_str(),
            nullptr,
            m_settings.boldFont ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            m_formatSizes[i],
            L"",
            &m_cachedFormats[i]);
        
        if (SUCCEEDED(hr)) {
            m_cachedFormats[i]->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            m_cachedFormats[i]->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }
    }
}

IDWriteTextFormat* MatrixRenderer::GetCachedFormat(float fontSize) {
    // Find the closest cached format
    auto it = std::lower_bound(m_formatSizes.begin(), m_formatSizes.end(), fontSize);
    
    // If exact match or very close, use it
    if (it != m_formatSizes.end()) {
        size_t index = std::distance(m_formatSizes.begin(), it);
        if (index < FONT_CACHE_SIZE && m_cachedFormats[index]) {
            return m_cachedFormats[index].Get();
        }
    }
    
    // If fontSize is larger than all cached sizes, use the largest
    if (fontSize > m_formatSizes.back() && m_cachedFormats[FONT_CACHE_SIZE - 1]) {
        return m_cachedFormats[FONT_CACHE_SIZE - 1].Get();
    }
    
    // If fontSize is smaller than all cached sizes, use the smallest
    if (fontSize < m_formatSizes[0] && m_cachedFormats[0]) {
        return m_cachedFormats[0].Get();
    }
    
    // Find closest match
    size_t closestIndex = 0;
    float minDiff = std::abs(fontSize - m_formatSizes[0]);
    for (size_t i = 1; i < FONT_CACHE_SIZE; ++i) {
        float diff = std::abs(fontSize - m_formatSizes[i]);
        if (diff < minDiff) {
            minDiff = diff;
            closestIndex = i;
        }
    }
    
    return m_cachedFormats[closestIndex] ? m_cachedFormats[closestIndex].Get() : nullptr;
}

GridCell& MatrixRenderer::GetCell(int x, int y) {
    uint64_t key = PackCoords(x, y);
    return m_sparseGrid[key];
}

bool MatrixRenderer::HasActiveCell(int x, int y) const {
    uint64_t key = PackCoords(x, y);
    auto it = m_sparseGrid.find(key);
    return it != m_sparseGrid.end() && it->second.isActive;
}

void MatrixRenderer::SetCellActive(int x, int y, const GridCell& cell) {
    uint64_t key = PackCoords(x, y);
    m_sparseGrid[key] = cell;
    
    // Add to active tracking if not already present
    if (m_activeCellSet.find(key) == m_activeCellSet.end()) {
        m_activeCellSet.insert(key);
        m_activeCells.push_back({x, y});
    }
}

void MatrixRenderer::DeactivateCell(int x, int y) {
    uint64_t key = PackCoords(x, y);
    
    // Remove from active tracking
    m_activeCellSet.erase(key);
    
    // Remove from active cells list
    auto it = std::find(m_activeCells.begin(), m_activeCells.end(), std::make_pair(x, y));
    if (it != m_activeCells.end()) {
        m_activeCells.erase(it);
    }
    
    // Remove from sparse grid
    m_sparseGrid.erase(key);
}

