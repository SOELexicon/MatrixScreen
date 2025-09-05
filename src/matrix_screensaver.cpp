#include "matrix_screensaver.h"

MatrixScreensaver::MatrixScreensaver() {
    m_renderer = std::make_unique<MatrixRenderer>();
    m_settingsManager = std::make_unique<SettingsManager>();
}

MatrixScreensaver::~MatrixScreensaver() {
    Shutdown();
}

bool MatrixScreensaver::Initialize(HWND hwnd) {
    m_hwnd = hwnd;
    
    // Load settings
    m_settings = m_settingsManager->LoadSettings();
    
    // Initialize renderer
    return m_renderer->Initialize(hwnd, m_settings);
}

void MatrixScreensaver::Shutdown() {
    if (m_renderer) {
        m_renderer->Shutdown();
    }
}

void MatrixScreensaver::Update(float deltaTime) {
    if (m_renderer) {
        m_renderer->Update(deltaTime);
    }
}

void MatrixScreensaver::Render() {
    if (m_renderer) {
        m_renderer->Render();
    }
}

void MatrixScreensaver::Resize(int width, int height) {
    if (m_renderer) {
        m_renderer->Resize(width, height);
    }
}