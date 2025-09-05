#include "settings_manager.h"
#include <sstream>

SettingsManager::SettingsManager() {
}

SettingsManager::~SettingsManager() {
}

MatrixSettings SettingsManager::LoadSettings() {
    MatrixSettings settings;
    
    HKEY hKey;
    LONG result = RegOpenKeyEx(HKEY_CURRENT_USER, REGISTRY_KEY, 0, KEY_READ, &hKey);
    
    if (result == ERROR_SUCCESS) {
        settings.speed = ReadFloat(hKey, L"Speed", 5.0f);
        settings.density = ReadFloat(hKey, L"Density", 0.6f);
        settings.messageSpeed = ReadFloat(hKey, L"MessageSpeed", 3.0f);
        settings.fontSize = ReadFloat(hKey, L"FontSize", 14.0f);
        settings.hue = ReadFloat(hKey, L"Hue", 120.0f);
        settings.randomizeMessages = ReadBool(hKey, L"RandomizeMessages", true);
        settings.boldFont = ReadBool(hKey, L"BoldFont", true);
        settings.fontName = ReadString(hKey, L"FontName", L"Consolas");
        settings.customWord = ReadString(hKey, L"CustomWord", L"MATRIX");
        settings.useCustomWord = ReadBool(hKey, L"UseCustomWord", false);
        settings.sequentialCharacters = ReadBool(hKey, L"SequentialCharacters", true);
        settings.showMaskBackground = ReadBool(hKey, L"ShowMaskBackground", false);
        settings.whiteHeadCharacters = ReadBool(hKey, L"WhiteHeadCharacters", true);
        settings.enable3DEffect = ReadBool(hKey, L"Enable3DEffect", true);
        settings.variableFontSize = ReadBool(hKey, L"VariableFontSize", true);
        settings.maskBackgroundOpacity = ReadFloat(hKey, L"MaskBackgroundOpacity", 0.3f);
        settings.maskImagePath = ReadString(hKey, L"MaskImagePath", L"");
        settings.useMask = ReadBool(hKey, L"UseMask", false);
        
        // Load custom messages
        std::wstring messagesStr = ReadString(hKey, L"CustomMessages", L"");
        if (!messagesStr.empty()) {
            std::wstringstream ss(messagesStr);
            std::wstring message;
            while (std::getline(ss, message, L'|')) {
                if (!message.empty()) {
                    settings.customMessages.push_back(message);
                }
            }
        }
        
        RegCloseKey(hKey);
    }
    
    return settings;
}

void SettingsManager::SaveSettings(const MatrixSettings& settings) {
    HKEY hKey;
    LONG result = RegCreateKeyEx(HKEY_CURRENT_USER, REGISTRY_KEY, 0, nullptr,
        REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
    
    if (result == ERROR_SUCCESS) {
        WriteFloat(hKey, L"Speed", settings.speed);
        WriteFloat(hKey, L"Density", settings.density);
        WriteFloat(hKey, L"MessageSpeed", settings.messageSpeed);
        WriteFloat(hKey, L"FontSize", settings.fontSize);
        WriteFloat(hKey, L"Hue", settings.hue);
        WriteBool(hKey, L"RandomizeMessages", settings.randomizeMessages);
        WriteBool(hKey, L"BoldFont", settings.boldFont);
        WriteString(hKey, L"FontName", settings.fontName);
        WriteString(hKey, L"CustomWord", settings.customWord);
        WriteBool(hKey, L"UseCustomWord", settings.useCustomWord);
        WriteBool(hKey, L"SequentialCharacters", settings.sequentialCharacters);
        WriteBool(hKey, L"ShowMaskBackground", settings.showMaskBackground);
        WriteBool(hKey, L"WhiteHeadCharacters", settings.whiteHeadCharacters);
        WriteBool(hKey, L"Enable3DEffect", settings.enable3DEffect);
        WriteBool(hKey, L"VariableFontSize", settings.variableFontSize);
        WriteFloat(hKey, L"MaskBackgroundOpacity", settings.maskBackgroundOpacity);
        WriteString(hKey, L"MaskImagePath", settings.maskImagePath);
        WriteBool(hKey, L"UseMask", settings.useMask);
        
        // Save custom messages
        std::wstring messagesStr;
        for (size_t i = 0; i < settings.customMessages.size(); ++i) {
            if (i > 0) messagesStr += L"|";
            messagesStr += settings.customMessages[i];
        }
        WriteString(hKey, L"CustomMessages", messagesStr);
        
        RegCloseKey(hKey);
    }
}

std::wstring SettingsManager::ReadString(HKEY hKey, const wchar_t* valueName, const wchar_t* defaultValue) {
    DWORD dataType;
    DWORD dataSize = 0;
    
    LONG result = RegQueryValueEx(hKey, valueName, nullptr, &dataType, nullptr, &dataSize);
    if (result != ERROR_SUCCESS || dataType != REG_SZ) {
        return defaultValue;
    }
    
    std::wstring value(dataSize / sizeof(wchar_t), 0);
    result = RegQueryValueEx(hKey, valueName, nullptr, &dataType,
        reinterpret_cast<LPBYTE>(value.data()), &dataSize);
    
    if (result == ERROR_SUCCESS) {
        // Remove null terminator if present
        if (!value.empty() && value.back() == 0) {
            value.pop_back();
        }
        return value;
    }
    
    return defaultValue;
}

DWORD SettingsManager::ReadDword(HKEY hKey, const wchar_t* valueName, DWORD defaultValue) {
    DWORD dataType;
    DWORD dataSize = sizeof(DWORD);
    DWORD value;
    
    LONG result = RegQueryValueEx(hKey, valueName, nullptr, &dataType,
        reinterpret_cast<LPBYTE>(&value), &dataSize);
    
    return (result == ERROR_SUCCESS && dataType == REG_DWORD) ? value : defaultValue;
}

float SettingsManager::ReadFloat(HKEY hKey, const wchar_t* valueName, float defaultValue) {
    DWORD dwordValue = ReadDword(hKey, valueName, *reinterpret_cast<const DWORD*>(&defaultValue));
    return *reinterpret_cast<float*>(&dwordValue);
}

bool SettingsManager::ReadBool(HKEY hKey, const wchar_t* valueName, bool defaultValue) {
    return ReadDword(hKey, valueName, defaultValue ? 1 : 0) != 0;
}

void SettingsManager::WriteString(HKEY hKey, const wchar_t* valueName, const std::wstring& value) {
    RegSetValueEx(hKey, valueName, 0, REG_SZ,
        reinterpret_cast<const BYTE*>(value.c_str()),
        static_cast<DWORD>((value.length() + 1) * sizeof(wchar_t)));
}

void SettingsManager::WriteDword(HKEY hKey, const wchar_t* valueName, DWORD value) {
    RegSetValueEx(hKey, valueName, 0, REG_DWORD,
        reinterpret_cast<const BYTE*>(&value), sizeof(DWORD));
}

void SettingsManager::WriteFloat(HKEY hKey, const wchar_t* valueName, float value) {
    WriteDword(hKey, valueName, *reinterpret_cast<DWORD*>(&value));
}

void SettingsManager::WriteBool(HKEY hKey, const wchar_t* valueName, bool value) {
    WriteDword(hKey, valueName, value ? 1 : 0);
}