#include "matrix_renderer.h"
#include "mask_loader.h"

MatrixRenderer::MatrixRenderer() 
    : m_lastUpdate(std::chrono::high_resolution_clock::now()) {
}

MatrixRenderer::~MatrixRenderer() {
    Shutdown();
}

bool MatrixRenderer::Initialize(HWND hwnd, const MatrixSettings& settings) {
    m_settings = settings;
    
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
    
    m_grid.clear();
    m_grid.resize(m_gridWidth, std::vector<GridCell>(m_gridHeight));
    
    // Initialize all grid cells
    for (int x = 0; x < m_gridWidth; ++x) {
        for (int y = 0; y < m_gridHeight; ++y) {
            m_grid[x][y].character = L"";
            m_grid[x][y].alpha = 0.0f;
            m_grid[x][y].fontSize = m_settings.fontSize;
            m_grid[x][y].depth = 0.5f;
            m_grid[x][y].isActive = false;
        }
    }
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
    return Color::FromHSV(m_settings.hue / 360.0f, 1.0f, 1.0f);
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
    // Create more classic Matrix colors with proper head/trail variation
    
    if (alpha > 0.9f) {
        // Head characters - bright white/green
        return Color(0.8f, 1.0f, 0.8f, alpha);
    } else if (depth > 0.7f) {
        // Mask areas - bright green  
        float intensity = 0.6f + 0.4f * alpha;
        return Color(0.0f, intensity, 0.0f, alpha);
    } else if (depth > 0.4f) {
        // Transition areas - medium green
        float intensity = 0.4f + 0.3f * alpha;
        return Color(0.0f, intensity, 0.0f, alpha);
    } else {
        // Outside/background areas - dark green
        float intensity = 0.2f + 0.3f * alpha;
        return Color(0.0f, intensity, 0.0f, alpha);
    }
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
    UpdateColumns(deltaTime);
}

void MatrixRenderer::UpdateColumns(float deltaTime) {
    std::uniform_int_distribution<int> charDist(0, static_cast<int>(MATRIX_CHARS.size()) - 1);
    
    for (auto& column : m_columns) {
        // Move column head down
        column.y += column.currentSpeed * deltaTime * 60.0f;
        
        // Calculate grid position
        int gridX = static_cast<int>(column.x / (m_settings.fontSize * 0.8f));
        int gridY = static_cast<int>(column.y / (m_settings.fontSize * 0.9f));
        
        // Check if head is in valid grid bounds
        if (gridX >= 0 && gridX < m_gridWidth && gridY >= 0 && gridY < m_gridHeight) {
            GridCell& cell = m_grid[gridX][gridY];
            
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
                } else {
                    // Always use random Japanese characters when not using custom word
                    cell.character = MATRIX_CHARS[charDist(g_rng)];
                }
                
                cell.alpha = 1.0f; // Start bright
                cell.fontSize = m_settings.fontSize * (0.7f + depth * 0.6f); // Depth-based size
                cell.depth = depth;
                cell.isActive = true;
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
    for (int x = 0; x < m_gridWidth; ++x) {
        for (int y = 0; y < m_gridHeight; ++y) {
            GridCell& cell = m_grid[x][y];
            
            if (cell.isActive) {
                // Fade the character over time
                float fadeRate = m_settings.fadeRate;
                cell.alpha -= fadeRate * deltaTime;
                
                // Deactivate when fully faded
                if (cell.alpha <= 0.0f) {
                    cell.alpha = 0.0f;
                    cell.isActive = false;
                    cell.character = L"";
                }
            }
        }
    }
}


void MatrixRenderer::Render() {
    // Clear background
    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_deviceContext->ClearRenderTargetView(m_renderTargetView.Get(), clearColor);
    
    m_d2dRenderTarget->BeginDraw();
    m_d2dRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black));
    
    // Render mask as lighter background if available and enabled
    if (m_maskBitmap && m_settings.useMask && m_settings.showMaskBackground) {
        RenderMaskBackground();
    }
    
    RenderGrid();
    RenderColumns();
    
    HRESULT hr = m_d2dRenderTarget->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
        // Handle device lost scenario
        InitializeDirect2D();
    }
    
    m_swapChain->Present(1, 0);
}

void MatrixRenderer::RenderGrid() {
    for (int x = 0; x < m_gridWidth; ++x) {
        for (int y = 0; y < m_gridHeight; ++y) {
            const GridCell& cell = m_grid[x][y];
            
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
            
            // Use dynamic text format if font size differs significantly
            if (std::abs(cell.fontSize - m_settings.fontSize) > 1.0f) {
                Microsoft::WRL::ComPtr<IDWriteTextFormat> dynamicFormat;
                HRESULT hr = m_writeFactory->CreateTextFormat(
                    m_settings.fontName.c_str(),
                    nullptr,
                    m_settings.boldFont ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL,
                    DWRITE_FONT_STYLE_NORMAL,
                    DWRITE_FONT_STRETCH_NORMAL,
                    cell.fontSize,
                    L"",
                    &dynamicFormat);
                
                if (SUCCEEDED(hr)) {
                    dynamicFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
                    dynamicFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
                    
                    m_d2dRenderTarget->DrawText(
                        cell.character.c_str(),
                        static_cast<UINT32>(cell.character.length()),
                        dynamicFormat.Get(),
                        layoutRect,
                        m_fadeBrush.Get());
                }
            } else {
                // Use default format
                m_d2dRenderTarget->DrawText(
                    cell.character.c_str(),
                    static_cast<UINT32>(cell.character.length()),
                    m_textFormat.Get(),
                    layoutRect,
                    m_fadeBrush.Get());
            }
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
        
        // Set color for head based on settings
        if (m_settings.whiteHeadCharacters) {
            m_whiteBrush->SetColor(D2D1::ColorF(D2D1::ColorF::White, 1.0f));
        } else {
            // Use bright green for heads instead of white
            m_whiteBrush->SetColor(D2D1::ColorF(0.0f, 1.0f, 0.0f, 1.0f));
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
    
    InitializeColumns();
}