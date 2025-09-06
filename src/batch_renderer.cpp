#include "batch_renderer.h"
#include "logger.h"

BatchRenderer::BatchRenderer() {
}

BatchRenderer::~BatchRenderer() {
}

void BatchRenderer::Initialize(size_t maxBatchSize) {
    m_maxBatchSize = maxBatchSize;
    m_batches.reserve(50); // Pre-allocate for common batch count
    
    LOG_DEBUG("BatchRenderer initialized with max batch size: " + std::to_string(maxBatchSize));
}

void BatchRenderer::Reset() {
    for (auto& [key, batch] : m_batches) {
        batch.Clear();
    }
    m_totalCharacters = 0;
}

void BatchRenderer::AddCharacter(const std::wstring& character,
                                const D2D1_RECT_F& position,
                                const D2D1_COLOR_F& color,
                                float fontSize) {
    if (!m_enabled) return;
    
    // Create batch key
    BatchKey key;
    key.colorHash = ColorToHash(color);
    key.fontSize = static_cast<int>(fontSize);
    
    // Get or create batch
    auto& batch = m_batches[key];
    
    // Initialize batch if new
    if (batch.positions.empty()) {
        batch.color = color;
        batch.fontSize = fontSize;
        batch.Reserve(m_maxBatchSize / 10); // Estimate characters per batch
    }
    
    // Add character to batch
    batch.text += character;
    batch.positions.push_back(position);
    m_totalCharacters++;
    
    // Flush if batch is full
    if (batch.positions.size() >= m_maxBatchSize) {
        // This batch is full, we'll flush it on next Flush() call
    }
}

void BatchRenderer::Flush(ID2D1RenderTarget* renderTarget,
                         IDWriteFactory* writeFactory,
                         IDWriteTextFormat* defaultFormat) {
    if (!m_enabled || !renderTarget || m_batches.empty()) {
        return;
    }
    
    size_t charactersRendered = 0;
    
    for (auto& [key, batch] : m_batches) {
        if (batch.positions.empty()) continue;
        
        // Get cached brush and format
        ID2D1SolidColorBrush* brush = GetOrCreateBrush(renderTarget, batch.color);
        IDWriteTextFormat* format = GetOrCreateFormat(writeFactory, batch.fontSize, defaultFormat);
        
        if (!brush || !format) continue;
        
        // For batch rendering, we can either:
        // 1. Draw each character individually (current approach but batched by color/size)
        // 2. Create a single text layout with positioned glyphs (more complex but faster)
        
        // Option 1: Individual character drawing (batched by properties)
        for (size_t i = 0; i < batch.positions.size() && i < batch.text.length(); ++i) {
            wchar_t singleChar[2] = { batch.text[i], L'\0' };
            
            renderTarget->DrawText(
                singleChar,
                1,
                format,
                &batch.positions[i],
                brush
            );
            
            charactersRendered++;
        }
    }
    
    if (charactersRendered > 0) {
        LOG_DEBUG("BatchRenderer flushed " + std::to_string(charactersRendered) + 
                 " characters in " + std::to_string(m_batches.size()) + " batches");
    }
    
    // Clear batches for next frame
    Reset();
}

UINT32 BatchRenderer::ColorToHash(const D2D1_COLOR_F& color) const {
    // Simple color hash - quantize to reduce cache misses
    UINT32 r = static_cast<UINT32>(color.r * 15) & 0xF;
    UINT32 g = static_cast<UINT32>(color.g * 15) & 0xF;
    UINT32 b = static_cast<UINT32>(color.b * 15) & 0xF;
    UINT32 a = static_cast<UINT32>(color.a * 15) & 0xF;
    
    return (r << 12) | (g << 8) | (b << 4) | a;
}

ID2D1SolidColorBrush* BatchRenderer::GetOrCreateBrush(ID2D1RenderTarget* renderTarget,
                                                     const D2D1_COLOR_F& color) {
    UINT32 hash = ColorToHash(color);
    
    auto it = m_brushCache.find(hash);
    if (it != m_brushCache.end()) {
        return it->second.Get();
    }
    
    // Create new brush
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
    HRESULT hr = renderTarget->CreateSolidColorBrush(color, &brush);
    
    if (SUCCEEDED(hr)) {
        m_brushCache[hash] = brush;
        return brush.Get();
    }
    
    return nullptr;
}

IDWriteTextFormat* BatchRenderer::GetOrCreateFormat(IDWriteFactory* writeFactory,
                                                   float fontSize,
                                                   IDWriteTextFormat* defaultFormat) {
    int sizeKey = static_cast<int>(fontSize);
    
    auto it = m_formatCache.find(sizeKey);
    if (it != m_formatCache.end()) {
        return it->second.Get();
    }
    
    if (!writeFactory || !defaultFormat) return defaultFormat;
    
    // Get properties from default format
    WCHAR fontName[256];
    defaultFormat->GetFontFamilyName(fontName, 256);
    
    DWRITE_FONT_WEIGHT weight = defaultFormat->GetFontWeight();
    DWRITE_FONT_STYLE style = defaultFormat->GetFontStyle();
    DWRITE_FONT_STRETCH stretch = defaultFormat->GetFontStretch();
    
    // Create new format with different size
    Microsoft::WRL::ComPtr<IDWriteTextFormat> format;
    HRESULT hr = writeFactory->CreateTextFormat(
        fontName,
        nullptr,
        weight,
        style,
        stretch,
        fontSize,
        L"",
        &format);
    
    if (SUCCEEDED(hr)) {
        format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        
        m_formatCache[sizeKey] = format;
        return format.Get();
    }
    
    return defaultFormat;
}