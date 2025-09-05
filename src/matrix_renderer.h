#pragma once

#include "common.h"

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
    
    // Mask resources
    Microsoft::WRL::ComPtr<ID2D1Bitmap> m_maskBitmap;
    std::vector<std::vector<float>> m_densityMap;
    
    // Animation data
    std::vector<MatrixColumn> m_columns;
    std::vector<std::vector<GridCell>> m_grid; // 2D grid of persistent characters
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
};