#include "config_dialog.h"
#include "resource.h"
#include <commdlg.h>
#include <commctrl.h>

ConfigDialog::ConfigDialog() {
    m_settingsManager = std::make_unique<SettingsManager>();
}

ConfigDialog::~ConfigDialog() {
}

bool ConfigDialog::Show(HINSTANCE hInstance) {
    m_settings = m_settingsManager->LoadSettings();
    
    INT_PTR result = DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_CONFIG),
        nullptr, DialogProc, reinterpret_cast<LPARAM>(this));
    
    return result == IDOK;
}

INT_PTR CALLBACK ConfigDialog::DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    static ConfigDialog* pThis = nullptr;
    
    switch (message) {
    case WM_INITDIALOG:
        pThis = reinterpret_cast<ConfigDialog*>(lParam);
        if (pThis) {
            return pThis->Initialize(hDlg) ? TRUE : FALSE;
        }
        return FALSE;
        
    case WM_COMMAND:
        if (pThis) {
            pThis->OnCommand(hDlg, wParam, lParam);
        }
        return TRUE;
        
    case WM_HSCROLL:
        if (pThis) {
            pThis->OnHScroll(hDlg, wParam, lParam);
        }
        return TRUE;
        
    case WM_CLOSE:
        EndDialog(hDlg, IDCANCEL);
        return TRUE;
    }
    
    return FALSE;
}

bool ConfigDialog::Initialize(HWND hDlg) {
    LoadSettingsToDialog(hDlg);
    UpdatePreview(hDlg);
    return true;
}

void ConfigDialog::LoadSettingsToDialog(HWND hDlg) {
    // Set slider positions
    SendDlgItemMessage(hDlg, IDC_SPEED_SLIDER, TBM_SETRANGE, 0, MAKELPARAM(1, 20));
    SendDlgItemMessage(hDlg, IDC_SPEED_SLIDER, TBM_SETPOS, TRUE, static_cast<LPARAM>(m_settings.speed));
    
    SendDlgItemMessage(hDlg, IDC_DENSITY_SLIDER, TBM_SETRANGE, 0, MAKELPARAM(10, 300));
    SendDlgItemMessage(hDlg, IDC_DENSITY_SLIDER, TBM_SETPOS, TRUE, static_cast<LPARAM>(m_settings.density * 100));
    
    SendDlgItemMessage(hDlg, IDC_FONTSIZE_SLIDER, TBM_SETRANGE, 0, MAKELPARAM(8, 24));
    SendDlgItemMessage(hDlg, IDC_FONTSIZE_SLIDER, TBM_SETPOS, TRUE, static_cast<LPARAM>(m_settings.fontSize));
    
    SendDlgItemMessage(hDlg, IDC_HUE_SLIDER, TBM_SETRANGE, 0, MAKELPARAM(0, 360));
    SendDlgItemMessage(hDlg, IDC_HUE_SLIDER, TBM_SETPOS, TRUE, static_cast<LPARAM>(m_settings.hue));
    
    SendDlgItemMessage(hDlg, IDC_MESSAGE_SPEED_SLIDER, TBM_SETRANGE, 0, MAKELPARAM(1, 10));
    SendDlgItemMessage(hDlg, IDC_MESSAGE_SPEED_SLIDER, TBM_SETPOS, TRUE, static_cast<LPARAM>(m_settings.messageSpeed));
    
    SendDlgItemMessage(hDlg, IDC_FADE_RATE_SLIDER, TBM_SETRANGE, 0, MAKELPARAM(1, 10));
    SendDlgItemMessage(hDlg, IDC_FADE_RATE_SLIDER, TBM_SETPOS, TRUE, static_cast<LPARAM>(m_settings.fadeRate));
    
    SendDlgItemMessage(hDlg, IDC_DEPTH_RANGE_SLIDER, TBM_SETRANGE, 0, MAKELPARAM(1, 20));
    SendDlgItemMessage(hDlg, IDC_DEPTH_RANGE_SLIDER, TBM_SETPOS, TRUE, static_cast<LPARAM>(m_settings.depthRange));
    
    // Set checkboxes
    CheckDlgButton(hDlg, IDC_BOLD_CHECK, m_settings.boldFont ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_RANDOMIZE_CHECK, m_settings.randomizeMessages ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_USE_CUSTOM_WORD, m_settings.useCustomWord ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_SEQUENTIAL_CHECK, m_settings.sequentialCharacters ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_SHOW_MASK_BG, m_settings.showMaskBackground ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_WHITE_HEAD_CHECK, m_settings.whiteHeadCharacters ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_ENABLE_3D_CHECK, m_settings.enable3DEffect ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_VARIABLE_FONT_CHECK, m_settings.variableFontSize ? BST_CHECKED : BST_UNCHECKED);
    
    // Set font combo
    HWND hFontCombo = GetDlgItem(hDlg, IDC_FONT_COMBO);
    SendMessage(hFontCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Consolas"));
    SendMessage(hFontCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Courier New"));
    SendMessage(hFontCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Lucida Console"));
    SendMessage(hFontCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Cascadia Code"));
    SendMessage(hFontCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Terminal"));
    
    // Select the saved font
    int selectedIndex = 0;
    if (m_settings.fontName == L"Courier New") {
        selectedIndex = 1;
    } else if (m_settings.fontName == L"Lucida Console") {
        selectedIndex = 2;
    } else if (m_settings.fontName == L"Cascadia Code") {
        selectedIndex = 3;
    } else if (m_settings.fontName == L"Terminal") {
        selectedIndex = 4;
    }
    // Default is 0 (Consolas)
    SendMessage(hFontCombo, CB_SETCURSEL, selectedIndex, 0);
    
    // Set custom word
    SetDlgItemText(hDlg, IDC_CUSTOM_WORD_EDIT, m_settings.customWord.c_str());
    
    // Set mask opacity slider
    SendDlgItemMessage(hDlg, IDC_MASK_OPACITY_SLIDER, TBM_SETRANGE, 0, MAKELPARAM(0, 100));
    SendDlgItemMessage(hDlg, IDC_MASK_OPACITY_SLIDER, TBM_SETPOS, TRUE, static_cast<LPARAM>(m_settings.maskBackgroundOpacity * 100));
    
    // Set mask path
    SetDlgItemText(hDlg, IDC_MASK_PATH, m_settings.maskImagePath.c_str());
    
    // Performance optimization settings (all OFF by default)
    CheckDlgButton(hDlg, IDC_ENABLE_BATCH_RENDERING, m_settings.enableBatchRendering ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_ENABLE_FRAME_LIMITING, m_settings.enableFrameRateLimiting ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_ENABLE_ADAPTIVE_VSYNC, m_settings.enableAdaptiveVSync ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_SHOW_PERFORMANCE_METRICS, m_settings.showPerformanceMetrics ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_ENABLE_DIRTY_RECTANGLES, m_settings.enableDirtyRectangles ? BST_CHECKED : BST_UNCHECKED);
    
    // Target FPS slider
    SendDlgItemMessage(hDlg, IDC_TARGET_FPS_SLIDER, TBM_SETRANGE, 0, MAKELPARAM(30, 144));
    SendDlgItemMessage(hDlg, IDC_TARGET_FPS_SLIDER, TBM_SETPOS, TRUE, static_cast<LPARAM>(m_settings.targetFrameRate));
    
    // Advanced features (all OFF by default)
    CheckDlgButton(hDlg, IDC_ENABLE_LOGGING, m_settings.enableLogging ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_ENABLE_MOTION_BLUR, m_settings.enableMotionBlur ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_ENABLE_PARTICLE_EFFECTS, m_settings.enableParticleEffects ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_ENABLE_AUDIO_VISUALIZATION, m_settings.enableAudioVisualization ? BST_CHECKED : BST_UNCHECKED);
    
    // Quality settings (all OFF by default)
    CheckDlgButton(hDlg, IDC_ENABLE_HIGH_QUALITY_TEXT, m_settings.enableHighQualityText ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_ENABLE_ANTI_ALIASING, m_settings.enableAntiAliasing ? BST_CHECKED : BST_UNCHECKED);
    
    // Visual enhancement features (all OFF by default)
    CheckDlgButton(hDlg, IDC_ENABLE_CHARACTER_MORPHING, m_settings.enableCharacterMorphing ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_ENABLE_PHOSPHOR_GLOW, m_settings.enablePhosphorGlow ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_ENABLE_GLITCH_EFFECTS, m_settings.enableGlitchEffects ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_ENABLE_RAIN_VARIATIONS, m_settings.enableRainVariations ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_ENABLE_SYSTEM_DISRUPTIONS, m_settings.enableSystemDisruptions ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_ENABLE_MOTION_REDUCTION, m_settings.enableMotionReduction ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_ENABLE_CHARACTER_VARIETY, m_settings.enableCharacterVariety ? BST_CHECKED : BST_UNCHECKED);
    
    // Enhancement parameter sliders
    SendDlgItemMessage(hDlg, IDC_MORPH_FREQUENCY_SLIDER, TBM_SETRANGE, 0, MAKELPARAM(1, 50));
    SendDlgItemMessage(hDlg, IDC_MORPH_FREQUENCY_SLIDER, TBM_SETPOS, TRUE, static_cast<LPARAM>(m_settings.morphFrequency * 100));
    
    SendDlgItemMessage(hDlg, IDC_MORPH_SPEED_SLIDER, TBM_SETRANGE, 0, MAKELPARAM(50, 500));
    SendDlgItemMessage(hDlg, IDC_MORPH_SPEED_SLIDER, TBM_SETPOS, TRUE, static_cast<LPARAM>(m_settings.morphSpeed * 100));
    
    SendDlgItemMessage(hDlg, IDC_GLITCH_FREQUENCY_SLIDER, TBM_SETRANGE, 0, MAKELPARAM(1, 20));
    SendDlgItemMessage(hDlg, IDC_GLITCH_FREQUENCY_SLIDER, TBM_SETPOS, TRUE, static_cast<LPARAM>(m_settings.glitchFrequency * 100));
    
    SendDlgItemMessage(hDlg, IDC_GLOW_INTENSITY_SLIDER, TBM_SETRANGE, 0, MAKELPARAM(0, 100));
    SendDlgItemMessage(hDlg, IDC_GLOW_INTENSITY_SLIDER, TBM_SETPOS, TRUE, static_cast<LPARAM>(m_settings.glowIntensity * 100));
    
    SendDlgItemMessage(hDlg, IDC_LATIN_CHAR_PROB_SLIDER, TBM_SETRANGE, 0, MAKELPARAM(0, 50));
    SendDlgItemMessage(hDlg, IDC_LATIN_CHAR_PROB_SLIDER, TBM_SETPOS, TRUE, static_cast<LPARAM>(m_settings.latinCharProbability * 100));
    
    SendDlgItemMessage(hDlg, IDC_SYMBOL_CHAR_PROB_SLIDER, TBM_SETRANGE, 0, MAKELPARAM(0, 20));
    SendDlgItemMessage(hDlg, IDC_SYMBOL_CHAR_PROB_SLIDER, TBM_SETPOS, TRUE, static_cast<LPARAM>(m_settings.symbolCharProbability * 100));
}

void ConfigDialog::SaveSettingsFromDialog(HWND hDlg) {
    m_settings.speed = static_cast<float>(SendDlgItemMessage(hDlg, IDC_SPEED_SLIDER, TBM_GETPOS, 0, 0));
    m_settings.density = static_cast<float>(SendDlgItemMessage(hDlg, IDC_DENSITY_SLIDER, TBM_GETPOS, 0, 0)) / 100.0f;
    m_settings.fontSize = static_cast<float>(SendDlgItemMessage(hDlg, IDC_FONTSIZE_SLIDER, TBM_GETPOS, 0, 0));
    m_settings.hue = static_cast<float>(SendDlgItemMessage(hDlg, IDC_HUE_SLIDER, TBM_GETPOS, 0, 0));
    m_settings.messageSpeed = static_cast<float>(SendDlgItemMessage(hDlg, IDC_MESSAGE_SPEED_SLIDER, TBM_GETPOS, 0, 0));
    m_settings.fadeRate = static_cast<float>(SendDlgItemMessage(hDlg, IDC_FADE_RATE_SLIDER, TBM_GETPOS, 0, 0));
    m_settings.depthRange = static_cast<float>(SendDlgItemMessage(hDlg, IDC_DEPTH_RANGE_SLIDER, TBM_GETPOS, 0, 0));
    
    m_settings.boldFont = IsDlgButtonChecked(hDlg, IDC_BOLD_CHECK) == BST_CHECKED;
    m_settings.randomizeMessages = IsDlgButtonChecked(hDlg, IDC_RANDOMIZE_CHECK) == BST_CHECKED;
    m_settings.useCustomWord = IsDlgButtonChecked(hDlg, IDC_USE_CUSTOM_WORD) == BST_CHECKED;
    m_settings.sequentialCharacters = IsDlgButtonChecked(hDlg, IDC_SEQUENTIAL_CHECK) == BST_CHECKED;
    m_settings.showMaskBackground = IsDlgButtonChecked(hDlg, IDC_SHOW_MASK_BG) == BST_CHECKED;
    m_settings.whiteHeadCharacters = IsDlgButtonChecked(hDlg, IDC_WHITE_HEAD_CHECK) == BST_CHECKED;
    m_settings.enable3DEffect = IsDlgButtonChecked(hDlg, IDC_ENABLE_3D_CHECK) == BST_CHECKED;
    m_settings.variableFontSize = IsDlgButtonChecked(hDlg, IDC_VARIABLE_FONT_CHECK) == BST_CHECKED;
    m_settings.maskBackgroundOpacity = static_cast<float>(SendDlgItemMessage(hDlg, IDC_MASK_OPACITY_SLIDER, TBM_GETPOS, 0, 0)) / 100.0f;
    
    // Get font name
    wchar_t fontName[256];
    GetDlgItemText(hDlg, IDC_FONT_COMBO, fontName, _countof(fontName));
    m_settings.fontName = fontName;
    
    // Get custom word
    wchar_t customWord[256];
    GetDlgItemText(hDlg, IDC_CUSTOM_WORD_EDIT, customWord, _countof(customWord));
    m_settings.customWord = customWord;
    
    // Get mask path
    wchar_t maskPath[MAX_PATH];
    GetDlgItemText(hDlg, IDC_MASK_PATH, maskPath, _countof(maskPath));
    m_settings.maskImagePath = maskPath;
    m_settings.useMask = !m_settings.maskImagePath.empty();
    
    // Performance optimization settings
    m_settings.enableBatchRendering = IsDlgButtonChecked(hDlg, IDC_ENABLE_BATCH_RENDERING) == BST_CHECKED;
    m_settings.enableFrameRateLimiting = IsDlgButtonChecked(hDlg, IDC_ENABLE_FRAME_LIMITING) == BST_CHECKED;
    m_settings.enableAdaptiveVSync = IsDlgButtonChecked(hDlg, IDC_ENABLE_ADAPTIVE_VSYNC) == BST_CHECKED;
    m_settings.showPerformanceMetrics = IsDlgButtonChecked(hDlg, IDC_SHOW_PERFORMANCE_METRICS) == BST_CHECKED;
    m_settings.enableDirtyRectangles = IsDlgButtonChecked(hDlg, IDC_ENABLE_DIRTY_RECTANGLES) == BST_CHECKED;
    
    // Target FPS
    m_settings.targetFrameRate = static_cast<int>(SendDlgItemMessage(hDlg, IDC_TARGET_FPS_SLIDER, TBM_GETPOS, 0, 0));
    
    // Advanced features
    m_settings.enableLogging = IsDlgButtonChecked(hDlg, IDC_ENABLE_LOGGING) == BST_CHECKED;
    m_settings.enableMotionBlur = IsDlgButtonChecked(hDlg, IDC_ENABLE_MOTION_BLUR) == BST_CHECKED;
    m_settings.enableParticleEffects = IsDlgButtonChecked(hDlg, IDC_ENABLE_PARTICLE_EFFECTS) == BST_CHECKED;
    m_settings.enableAudioVisualization = IsDlgButtonChecked(hDlg, IDC_ENABLE_AUDIO_VISUALIZATION) == BST_CHECKED;
    
    // Quality settings
    m_settings.enableHighQualityText = IsDlgButtonChecked(hDlg, IDC_ENABLE_HIGH_QUALITY_TEXT) == BST_CHECKED;
    m_settings.enableAntiAliasing = IsDlgButtonChecked(hDlg, IDC_ENABLE_ANTI_ALIASING) == BST_CHECKED;
    
    // Visual enhancement features
    m_settings.enableCharacterMorphing = IsDlgButtonChecked(hDlg, IDC_ENABLE_CHARACTER_MORPHING) == BST_CHECKED;
    m_settings.enablePhosphorGlow = IsDlgButtonChecked(hDlg, IDC_ENABLE_PHOSPHOR_GLOW) == BST_CHECKED;
    m_settings.enableGlitchEffects = IsDlgButtonChecked(hDlg, IDC_ENABLE_GLITCH_EFFECTS) == BST_CHECKED;
    m_settings.enableRainVariations = IsDlgButtonChecked(hDlg, IDC_ENABLE_RAIN_VARIATIONS) == BST_CHECKED;
    m_settings.enableSystemDisruptions = IsDlgButtonChecked(hDlg, IDC_ENABLE_SYSTEM_DISRUPTIONS) == BST_CHECKED;
    m_settings.enableMotionReduction = IsDlgButtonChecked(hDlg, IDC_ENABLE_MOTION_REDUCTION) == BST_CHECKED;
    m_settings.enableCharacterVariety = IsDlgButtonChecked(hDlg, IDC_ENABLE_CHARACTER_VARIETY) == BST_CHECKED;
    
    // Enhancement parameters
    m_settings.morphFrequency = static_cast<float>(SendDlgItemMessage(hDlg, IDC_MORPH_FREQUENCY_SLIDER, TBM_GETPOS, 0, 0)) / 100.0f;
    m_settings.morphSpeed = static_cast<float>(SendDlgItemMessage(hDlg, IDC_MORPH_SPEED_SLIDER, TBM_GETPOS, 0, 0)) / 100.0f;
    m_settings.glitchFrequency = static_cast<float>(SendDlgItemMessage(hDlg, IDC_GLITCH_FREQUENCY_SLIDER, TBM_GETPOS, 0, 0)) / 100.0f;
    m_settings.glowIntensity = static_cast<float>(SendDlgItemMessage(hDlg, IDC_GLOW_INTENSITY_SLIDER, TBM_GETPOS, 0, 0)) / 100.0f;
    m_settings.latinCharProbability = static_cast<float>(SendDlgItemMessage(hDlg, IDC_LATIN_CHAR_PROB_SLIDER, TBM_GETPOS, 0, 0)) / 100.0f;
    m_settings.symbolCharProbability = static_cast<float>(SendDlgItemMessage(hDlg, IDC_SYMBOL_CHAR_PROB_SLIDER, TBM_GETPOS, 0, 0)) / 100.0f;
    
    m_settingsManager->SaveSettings(m_settings);
}

void ConfigDialog::OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam) {
    UNREFERENCED_PARAMETER(lParam);
    
    switch (LOWORD(wParam)) {
    case IDOK:
        SaveSettingsFromDialog(hDlg);
        EndDialog(hDlg, IDOK);
        break;
        
    case IDCANCEL:
        EndDialog(hDlg, IDCANCEL);
        break;
        
    case IDC_BROWSE_MASK:
        BrowseForMaskImage(hDlg);
        break;
        
    case IDC_CLEAR_MASK:
        SetDlgItemText(hDlg, IDC_MASK_PATH, L"");
        UpdatePreview(hDlg);
        break;
        
    default:
        if (HIWORD(wParam) == CBN_SELCHANGE || HIWORD(wParam) == BN_CLICKED) {
            UpdatePreview(hDlg);
        }
        break;
    }
}

void ConfigDialog::OnHScroll(HWND hDlg, WPARAM wParam, LPARAM lParam) {
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    UpdatePreview(hDlg);
}

void ConfigDialog::UpdatePreview(HWND hDlg) {
    // Update preview window - for now just a placeholder
    UNREFERENCED_PARAMETER(hDlg);
    // TODO: Implement live preview
}

void ConfigDialog::BrowseForMaskImage(HWND hDlg) {
    OPENFILENAME ofn = {};
    wchar_t szFile[MAX_PATH] = {};
    
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hDlg;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile) / sizeof(wchar_t);
    ofn.lpstrFilter = L"Image Files\0*.png;*.jpg;*.jpeg;*.bmp\0PNG Files\0*.png\0JPEG Files\0*.jpg;*.jpeg\0Bitmap Files\0*.bmp\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = nullptr;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    
    if (GetOpenFileName(&ofn)) {
        SetDlgItemText(hDlg, IDC_MASK_PATH, szFile);
        UpdatePreview(hDlg);
    }
}