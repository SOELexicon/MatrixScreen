#pragma once

#include "common.h"
#include <vector>
#include <unordered_map>

struct CharacterBatch {
    std::wstring text;
    std::vector<D2D1_RECT_F> positions;
    D2D1_COLOR_F color;
    float fontSize;
    
    void Clear() {
        text.clear();
        positions.clear();
    }
    
    void Reserve(size_t capacity) {
        text.reserve(capacity);
        positions.reserve(capacity);
    }
};

class BatchRenderer {
public:
    BatchRenderer();
    ~BatchRenderer();
    
    void Initialize(size_t maxBatchSize = 1000);
    void Reset();
    
    // Add a character to the batch
    void AddCharacter(const std::wstring& character, 
                     const D2D1_RECT_F& position,
                     const D2D1_COLOR_F& color,
                     float fontSize);
    
    // Flush all batches to the render target
    void Flush(ID2D1RenderTarget* renderTarget,
              IDWriteFactory* writeFactory,
              IDWriteTextFormat* defaultFormat);
    
    size_t GetBatchCount() const { return m_batches.size(); }
    size_t GetTotalCharacters() const { return m_totalCharacters; }
    
    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

private:
    struct BatchKey {
        UINT32 colorHash;
        int fontSize;
        
        bool operator==(const BatchKey& other) const {
            return colorHash == other.colorHash && fontSize == other.fontSize;
        }
    };
    
    struct BatchKeyHash {
        size_t operator()(const BatchKey& key) const {
            return std::hash<UINT32>()(key.colorHash) ^ 
                   (std::hash<int>()(key.fontSize) << 1);
        }
    };
    
    bool m_enabled = false;
    size_t m_maxBatchSize = 1000;
    size_t m_totalCharacters = 0;
    
    // Batches grouped by color and font size
    std::unordered_map<BatchKey, CharacterBatch, BatchKeyHash> m_batches;
    
    // Cached brushes for different colors
    std::unordered_map<UINT32, Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>> m_brushCache;
    
    // Cached text formats for different font sizes
    std::unordered_map<int, Microsoft::WRL::ComPtr<IDWriteTextFormat>> m_formatCache;
    
    UINT32 ColorToHash(const D2D1_COLOR_F& color) const;
    ID2D1SolidColorBrush* GetOrCreateBrush(ID2D1RenderTarget* renderTarget, 
                                           const D2D1_COLOR_F& color);
    IDWriteTextFormat* GetOrCreateFormat(IDWriteFactory* writeFactory,
                                        float fontSize,
                                        IDWriteTextFormat* defaultFormat);
};