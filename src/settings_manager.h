#pragma once

#include "common.h"

class SettingsManager {
public:
    SettingsManager();
    ~SettingsManager();

    MatrixSettings LoadSettings();
    void SaveSettings(const MatrixSettings& settings);

private:
    static constexpr const wchar_t* REGISTRY_KEY = L"SOFTWARE\\MatrixScreensaver";
    
    std::wstring ReadString(HKEY hKey, const wchar_t* valueName, const wchar_t* defaultValue);
    DWORD ReadDword(HKEY hKey, const wchar_t* valueName, DWORD defaultValue);
    float ReadFloat(HKEY hKey, const wchar_t* valueName, float defaultValue);
    bool ReadBool(HKEY hKey, const wchar_t* valueName, bool defaultValue);
    
    void WriteString(HKEY hKey, const wchar_t* valueName, const std::wstring& value);
    void WriteDword(HKEY hKey, const wchar_t* valueName, DWORD value);
    void WriteFloat(HKEY hKey, const wchar_t* valueName, float value);
    void WriteBool(HKEY hKey, const wchar_t* valueName, bool value);
};