#pragma once

#include "common.h"
#include <deque>

class PerformanceMetrics {
public:
    PerformanceMetrics();
    ~PerformanceMetrics();

    void StartFrame();
    void EndFrame();
    void Render(ID2D1RenderTarget* renderTarget, IDWriteFactory* writeFactory);
    
    float GetCurrentFPS() const { return m_currentFPS; }
    float GetAverageFPS() const { return m_averageFPS; }
    float GetFrameTime() const { return m_frameTime; }
    
    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

private:
    bool m_enabled = false;
    
    // Timing data
    std::chrono::high_resolution_clock::time_point m_frameStartTime;
    std::chrono::high_resolution_clock::time_point m_lastFrameTime;
    
    // FPS tracking
    float m_currentFPS = 0.0f;
    float m_averageFPS = 0.0f;
    float m_frameTime = 0.0f;
    
    // History for averaging
    std::deque<float> m_fpsHistory;
    static constexpr size_t FPS_HISTORY_SIZE = 60;
    
    // Rendering resources
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_textBrush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_backgroundBrush;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> m_textFormat;
    
    // Update counter
    int m_updateCounter = 0;
    static constexpr int UPDATE_FREQUENCY = 10; // Update display every N frames
};