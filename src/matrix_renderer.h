#pragma once

#include "common.h"
#include <unordered_map>
#include <unordered_set>
#include <array>
#include <algorithm>

class MatrixRenderer {
public:
    MatrixRenderer();
    ~MatrixRenderer();

    bool Initialize(HWND hwnd, const MatrixSettings& settings);
    void Shutdown();
    void Update(float deltaTime);
    void Render();
    void Resize(int width, int height);
    void UpdateSettings(const MatrixSettings& settings);
    void LoadMask(const std::wstring& imagePath);

private:
    // DirectX resources
    Microsoft::WRL::ComPtr<ID3D11Device> m_device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_deviceContext;
    Microsoft::WRL::ComPtr<IDXGISwapChain> m_swapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_renderTargetView;
    
    // Direct2D resources
    Microsoft::WRL::ComPtr<ID2D1Factory> m_d2dFactory;
    Microsoft::WRL::ComPtr<ID2D1RenderTarget> m_d2dRenderTarget;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_greenBrush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_whiteBrush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_fadeBrush;
    
    // DirectWrite resources
    Microsoft::WRL::ComPtr<IDWriteFactory> m_writeFactory;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> m_textFormat;
    
    // Font format cache for performance
    static constexpr size_t FONT_CACHE_SIZE = 10;
    std::array<Microsoft::WRL::ComPtr<IDWriteTextFormat>, FONT_CACHE_SIZE> m_cachedFormats;
    std::array<float, FONT_CACHE_SIZE> m_formatSizes = {8.0f, 10.0f, 12.0f, 14.0f, 16.0f, 18.0f, 20.0f, 24.0f, 28.0f, 32.0f};
    
    // Mask resources
    Microsoft::WRL::ComPtr<ID2D1Bitmap> m_maskBitmap;
    std::vector<std::vector<float>> m_densityMap;
    
    // Animation data
    std::vector<MatrixColumn> m_columns;
    
    // Optimized sparse grid storage
    std::unordered_map<uint64_t, GridCell> m_sparseGrid;  // Sparse grid for memory efficiency
    std::vector<std::pair<int, int>> m_activeCells;       // List of active cells to render
    std::unordered_set<uint64_t> m_activeCellSet;         // Fast lookup for active cells
    
    int m_gridWidth = 0;
    int m_gridHeight = 0;
    MatrixSettings m_settings;
    int m_screenWidth = 0;
    int m_screenHeight = 0;
    
    // Timing
    std::chrono::high_resolution_clock::time_point m_lastUpdate;
    
    // Private methods
    bool InitializeDirect3D(HWND hwnd);
    bool InitializeDirect2D();
    bool InitializeDirectWrite();
    void InitializeColumns();
    void InitializeGrid();
    void CreateDensityMap();
    void UpdateColumns(float deltaTime);
    void UpdateGrid(float deltaTime);
    float GetDensityAt(int x, int y) const;
    float GetMaskBrightness(int x, int y) const; // Get brightness from mask for 3D depth
    void RenderGrid();
    void RenderColumns();
    void RenderMaskBackground();
    Color GetMatrixColor() const;
    Color GetDepthColor(float depth, float alpha) const; // Color based on depth
    
    // Optimization helpers
    inline uint64_t PackCoords(int x, int y) const { 
        return (static_cast<uint64_t>(x) << 32) | static_cast<uint64_t>(y); 
    }
    inline std::pair<int, int> UnpackCoords(uint64_t packed) const {
        return { static_cast<int>(packed >> 32), static_cast<int>(packed & 0xFFFFFFFF) };
    }
    GridCell& GetCell(int x, int y);
    bool HasActiveCell(int x, int y) const;
    void SetCellActive(int x, int y, const GridCell& cell);
    void DeactivateCell(int x, int y);
    IDWriteTextFormat* GetCachedFormat(float fontSize);
    void InitializeFontCache();
};