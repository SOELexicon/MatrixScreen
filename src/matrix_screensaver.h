#pragma once

#include "common.h"
#include "matrix_renderer.h"
#include "settings_manager.h"

class MatrixScreensaver {
public:
    MatrixScreensaver();
    ~MatrixScreensaver();

    bool Initialize(HWND hwnd);
    void Shutdown();
    void Update(float deltaTime);
    void Render();
    void Resize(int width, int height);

private:
    std::unique_ptr<MatrixRenderer> m_renderer;
    std::unique_ptr<SettingsManager> m_settingsManager;
    MatrixSettings m_settings;
    HWND m_hwnd = nullptr;
};