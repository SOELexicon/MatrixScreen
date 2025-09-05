#pragma once

#include "common.h"

struct BitmapData {
    std::vector<uint8_t> pixels;
    int width = 0;
    int height = 0;
    int channels = 4; // RGBA
};

class MaskLoader {
public:
    MaskLoader();
    ~MaskLoader();

    bool LoadFromFile(const std::wstring& filePath);
    const BitmapData& GetBitmapData() const { return m_bitmapData; }
    
    // Convert to density map (0.0 = transparent/black, 1.0 = opaque/white)
    std::vector<std::vector<float>> CreateDensityMap(int targetWidth, int targetHeight) const;

private:
    bool InitializeWIC();
    void Cleanup();
    
    Microsoft::WRL::ComPtr<IWICImagingFactory> m_wicFactory;
    BitmapData m_bitmapData;
};