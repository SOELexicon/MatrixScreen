#include "mask_loader.h"

MaskLoader::MaskLoader() {
    InitializeWIC();
}

MaskLoader::~MaskLoader() {
    Cleanup();
}

bool MaskLoader::InitializeWIC() {
    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&m_wicFactory));
    
    return SUCCEEDED(hr);
}

void MaskLoader::Cleanup() {
    m_bitmapData = {};
}

bool MaskLoader::LoadFromFile(const std::wstring& filePath) {
    if (!m_wicFactory) {
        if (!InitializeWIC()) return false;
    }
    
    // Create decoder
    Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
    HRESULT hr = m_wicFactory->CreateDecoderFromFilename(
        filePath.c_str(),
        nullptr,
        GENERIC_READ,
        WICDecodeMetadataCacheOnLoad,
        &decoder);
    
    if (FAILED(hr)) return false;
    
    // Get first frame
    Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
    hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr)) return false;
    
    // Get image dimensions
    UINT width, height;
    hr = frame->GetSize(&width, &height);
    if (FAILED(hr)) return false;
    
    // Convert to RGBA format
    Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
    hr = m_wicFactory->CreateFormatConverter(&converter);
    if (FAILED(hr)) return false;
    
    hr = converter->Initialize(
        frame.Get(),
        GUID_WICPixelFormat32bppRGBA,
        WICBitmapDitherTypeNone,
        nullptr,
        0.0,
        WICBitmapPaletteTypeMedianCut);
    
    if (FAILED(hr)) return false;
    
    // Copy pixel data
    UINT stride = width * 4; // 4 bytes per pixel (RGBA)
    UINT bufferSize = stride * height;
    
    m_bitmapData.pixels.resize(bufferSize);
    hr = converter->CopyPixels(
        nullptr,
        stride,
        bufferSize,
        m_bitmapData.pixels.data());
    
    if (FAILED(hr)) return false;
    
    m_bitmapData.width = static_cast<int>(width);
    m_bitmapData.height = static_cast<int>(height);
    m_bitmapData.channels = 4;
    
    return true;
}

std::vector<std::vector<float>> MaskLoader::CreateDensityMap(int targetWidth, int targetHeight) const {
    std::vector<std::vector<float>> densityMap(targetWidth, std::vector<float>(targetHeight, 0.5f));
    
    if (m_bitmapData.pixels.empty() || m_bitmapData.width == 0 || m_bitmapData.height == 0) {
        return densityMap;
    }
    
    for (int y = 0; y < targetHeight; ++y) {
        for (int x = 0; x < targetWidth; ++x) {
            // Map target coordinates to source coordinates
            int srcX = static_cast<int>((static_cast<float>(x) / targetWidth) * m_bitmapData.width);
            int srcY = static_cast<int>((static_cast<float>(y) / targetHeight) * m_bitmapData.height);
            
            // Clamp to valid range
            srcX = std::clamp(srcX, 0, m_bitmapData.width - 1);
            srcY = std::clamp(srcY, 0, m_bitmapData.height - 1);
            
            // Get pixel from source image
            int pixelIndex = (srcY * m_bitmapData.width + srcX) * 4;
            
            if (pixelIndex + 3 < static_cast<int>(m_bitmapData.pixels.size())) {
                uint8_t r = m_bitmapData.pixels[pixelIndex + 0];
                uint8_t g = m_bitmapData.pixels[pixelIndex + 1];
                uint8_t b = m_bitmapData.pixels[pixelIndex + 2];
                uint8_t a = m_bitmapData.pixels[pixelIndex + 3];
                
                // Calculate luminance (brightness)
                float luminance = (0.299f * r + 0.587f * g + 0.114f * b) / 255.0f;
                
                // Apply alpha
                float alpha = a / 255.0f;
                luminance *= alpha;
                
                // Map luminance to density (brighter = higher density)
                densityMap[x][y] = std::clamp(luminance, 0.1f, 1.0f);
            }
        }
    }
    
    return densityMap;
}