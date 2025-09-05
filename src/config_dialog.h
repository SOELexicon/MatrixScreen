#pragma once

#include "common.h"
#include "settings_manager.h"

class ConfigDialog {
public:
    ConfigDialog();
    ~ConfigDialog();

    bool Show(HINSTANCE hInstance);

private:
    static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    
    bool Initialize(HWND hDlg);
    void LoadSettingsToDialog(HWND hDlg);
    void SaveSettingsFromDialog(HWND hDlg);
    void OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam);
    void OnHScroll(HWND hDlg, WPARAM wParam, LPARAM lParam);
    void UpdatePreview(HWND hDlg);
    void BrowseForMaskImage(HWND hDlg);
    
    std::unique_ptr<SettingsManager> m_settingsManager;
    MatrixSettings m_settings;
};