#include "performance_metrics.h"
#include <sstream>
#include <iomanip>

PerformanceMetrics::PerformanceMetrics() 
    : m_lastFrameTime(std::chrono::high_resolution_clock::now()) {
}

PerformanceMetrics::~PerformanceMetrics() {
}

void PerformanceMetrics::StartFrame() {
    if (!m_enabled) return;
    m_frameStartTime = std::chrono::high_resolution_clock::now();
}

void PerformanceMetrics::EndFrame() {
    if (!m_enabled) return;
    
    auto now = std::chrono::high_resolution_clock::now();
    auto frameDuration = std::chrono::duration<float, std::milli>(now - m_frameStartTime);
    m_frameTime = frameDuration.count();
    
    // Calculate instant FPS
    auto deltaTime = std::chrono::duration<float>(now - m_lastFrameTime);
    if (deltaTime.count() > 0) {
        m_currentFPS = 1.0f / deltaTime.count();
        
        // Add to history
        m_fpsHistory.push_back(m_currentFPS);
        if (m_fpsHistory.size() > FPS_HISTORY_SIZE) {
            m_fpsHistory.pop_front();
        }
        
        // Calculate average
        if (!m_fpsHistory.empty()) {
            float sum = 0;
            for (float fps : m_fpsHistory) {
                sum += fps;
            }
            m_averageFPS = sum / m_fpsHistory.size();
        }
    }
    
    m_lastFrameTime = now;
}

void PerformanceMetrics::Render(ID2D1RenderTarget* renderTarget, IDWriteFactory* writeFactory) {
    if (!m_enabled || !renderTarget || !writeFactory) return;
    
    // Only update display every N frames to reduce flicker
    m_updateCounter++;
    if (m_updateCounter < UPDATE_FREQUENCY) {
        return;
    }
    m_updateCounter = 0;
    
    // Create resources if needed
    if (!m_textBrush) {
        renderTarget->CreateSolidColorBrush(
            D2D1::ColorF(0.0f, 1.0f, 0.0f, 1.0f), // Bright green
            &m_textBrush);
    }
    
    if (!m_backgroundBrush) {
        renderTarget->CreateSolidColorBrush(
            D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.7f), // Semi-transparent black
            &m_backgroundBrush);
    }
    
    if (!m_textFormat) {
        writeFactory->CreateTextFormat(
            L"Consolas",
            nullptr,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            12.0f,
            L"",
            &m_textFormat);
        
        if (m_textFormat) {
            m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
            m_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
        }
    }
    
    // Format the display text
    std::wstringstream ss;
    ss << L"FPS: " << std::fixed << std::setprecision(1) << m_currentFPS;
    ss << L" (Avg: " << std::fixed << std::setprecision(1) << m_averageFPS << L")\n";
    ss << L"Frame Time: " << std::fixed << std::setprecision(2) << m_frameTime << L" ms";
    
    std::wstring text = ss.str();
    
    // Draw background
    D2D1_RECT_F bgRect = D2D1::RectF(5, 5, 200, 45);
    renderTarget->FillRectangle(&bgRect, m_backgroundBrush.Get());
    
    // Draw text
    D2D1_RECT_F textRect = D2D1::RectF(10, 10, 195, 40);
    renderTarget->DrawText(
        text.c_str(),
        static_cast<UINT32>(text.length()),
        m_textFormat.Get(),
        textRect,
        m_textBrush.Get());
}