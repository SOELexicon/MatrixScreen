#pragma once

#include <windows.h>
#include <d3d11.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <comdef.h>
#include <wrl/client.h>

#include <memory>
#include <vector>
#include <string>
#include <random>
#include <chrono>
#include <array>
#include <optional>
#include <algorithm>

// Don't use "using namespace" to avoid conflicts
// Modern C++20 utilities

// Safe release macro
template<typename T>
void SafeRelease(T*& ptr) {
    if (ptr) {
        ptr->Release();
        ptr = nullptr;
    }
}

// Matrix settings structure
struct MatrixSettings {
    float speed = 5.0f;
    float density = 0.8f;
    float messageSpeed = 3.0f;
    float fontSize = 14.0f;
    float minFontSize = 8.0f; // Far depth (dark mask areas)
    float maxFontSize = 28.0f; // Near depth (bright mask areas)  
    float minSpeed = 2.0f; // Far depth speed
    float maxSpeed = 10.0f; // Near depth speed
    float depthRange = 5.0f; // How dramatic the 3D depth effect is
    float hue = 120.0f;
    bool randomizeMessages = true;
    bool boldFont = true;
    bool enable3DEffect = true; // Enable 3D depth mapping
    bool variableFontSize = true;
    bool persistentCharacters = true;
    bool useCustomWord = false; // Use custom word in mask areas
    bool sequentialCharacters = true; // Use sequential characters instead of random
    bool showMaskBackground = false; // Show mask image as background
    bool whiteHeadCharacters = true; // Use bright white for leading characters
    float maskBackgroundOpacity = 0.3f; // Opacity of background mask (0.0-1.0)
    float fadeRate = 2.0f;
    std::wstring fontName = L"Consolas";
    std::wstring customWord = L"MATRIX"; // Custom word to display in mask areas
    std::vector<std::wstring> customMessages;
    std::wstring maskImagePath;
    bool useMask = false;
    
    // Performance optimization features (all OFF by default)
    bool enableBatchRendering = false; // Batch character rendering for better performance
    bool enableFrameRateLimiting = false; // Limit frame rate to reduce CPU/GPU usage
    int targetFrameRate = 60; // Target FPS when frame limiting is enabled
    bool enableAdaptiveVSync = false; // Adaptive VSync for smoother rendering
    bool showPerformanceMetrics = false; // Show FPS counter and performance stats
    bool enableDirtyRectangles = false; // Only redraw changed screen regions
    
    // Advanced features (all OFF by default)
    bool enableLogging = false; // Enable debug logging to file
    bool enableMotionBlur = false; // Add motion blur effect to falling characters
    bool enableParticleEffects = false; // Add glowing particles and sparks
    bool enableAudioVisualization = false; // React to system audio levels
    
    // Quality settings
    bool enableHighQualityText = false; // Use subpixel text rendering
    bool enableAntiAliasing = false; // Enable anti-aliasing for smoother edges
    
    // Visual enhancement features (all OFF by default)
    bool enableCharacterMorphing = false; // Characters morph/change while falling
    bool enablePhosphorGlow = false; // Add subtle glow around characters
    bool enableGlitchEffects = false; // Occasional character glitches
    bool enableRainVariations = false; // Vary rain intensity over time
    bool enableSystemDisruptions = false; // Occasional screen flickers
    bool enableMotionReduction = false; // Reduce animation for accessibility
    
    // Morphing settings
    float morphFrequency = 0.1f; // How often characters morph (0.0-1.0)
    float morphSpeed = 2.0f; // Speed of morphing animation
    float glitchFrequency = 0.05f; // How often glitches occur
    float glowIntensity = 0.3f; // Intensity of phosphor glow
    
    // Character variety settings
    float latinCharProbability = 0.15f; // 15% chance of Latin chars
    float symbolCharProbability = 0.05f; // 5% chance of symbols
    bool enableCharacterVariety = true; // Use expanded character set
};

// Color utilities
struct Color {
    float r, g, b, a;
    
    Color(float red = 0.0f, float green = 0.0f, float blue = 0.0f, float alpha = 1.0f)
        : r(red), g(green), b(blue), a(alpha) {}
    
    static Color FromHSV(float h, float s, float v, float a = 1.0f) {
        float c = v * s;
        float x = c * (1.0f - static_cast<float>(std::fabs(std::fmod(h / 60.0f, 2.0f) - 1.0f)));
        float m = v - c;
        
        float r, g, b;
        
        if (h >= 0 && h < 60) {
            r = c; g = x; b = 0;
        } else if (h >= 60 && h < 120) {
            r = x; g = c; b = 0;
        } else if (h >= 120 && h < 180) {
            r = 0; g = c; b = x;
        } else if (h >= 180 && h < 240) {
            r = 0; g = x; b = c;
        } else if (h >= 240 && h < 300) {
            r = x; g = 0; b = c;
        } else {
            r = c; g = 0; b = x;
        }
        
        return Color(r + m, g + m, b + m, a);
    }
    
    D2D1_COLOR_F ToD2D1() const { return {r, g, b, a}; }
};

// Screen grid cell for persistent Matrix effect
struct GridCell {
    std::wstring character;
    std::wstring morphTarget = L""; // Character to morph into
    float alpha = 0.0f;
    float fontSize = 14.0f;
    float depth = 0.5f;
    bool isActive = false;
    float lastUpdateTime = 0.0f;
    
    // Morphing animation
    float morphProgress = 0.0f;     // 0.0 = original, 1.0 = target
    float morphSpeed = 0.0f;        // How fast to morph
    float morphTimer = 0.0f;        // Time since morph started
    bool isMorphing = false;
    
    // Glitch effects
    float glitchIntensity = 0.0f;   // 0.0 = no glitch, 1.0 = full glitch
    float glitchTimer = 0.0f;
    bool isGlitching = false;
    
    // Phosphor glow
    float glowIntensity = 0.0f;     // Additional glow around character
    Color glowColor = Color(0.0f, 1.0f, 0.0f, 0.0f);
};

// Matrix column structure for moving heads
struct MatrixColumn {
    float x, y;
    float baseSpeed; // Base speed for this column
    float currentSpeed; // Current speed (varies with depth)
    float baseFontSize; // Base font size for this column layer
    int layer; // Which layer this column belongs to (0=large, 1=medium, 2=small)
    int customWordIndex = 0; // Current index in custom word for this column
    float alpha = 1.0f;
    bool isActive = true; // Whether this column is currently dropping
};

// Matrix characters - expanded authentic set with morphing capability
const std::vector<std::wstring> MATRIX_CHARS = {
    // Katakana (main characters from movie)
    L"ア", L"イ", L"ウ", L"エ", L"オ", L"カ", L"キ", L"ク", L"ケ", L"コ", L"サ", L"シ", L"ス", L"セ", L"ソ",
    L"タ", L"チ", L"ツ", L"テ", L"ト", L"ナ", L"ニ", L"ヌ", L"ネ", L"ノ", L"ハ", L"ヒ", L"フ", L"ヘ", L"ホ",
    L"マ", L"ミ", L"ム", L"メ", L"モ", L"ヤ", L"ユ", L"ヨ", L"ラ", L"リ", L"ル", L"レ", L"ロ", L"ワ", L"ヲ", L"ン",
    
    // Additional Katakana for more variety
    L"ァ", L"ィ", L"ゥ", L"ェ", L"ォ", L"ガ", L"ギ", L"グ", L"ゲ", L"ゴ", L"ザ", L"ジ", L"ズ", L"ゼ", L"ゾ",
    L"ダ", L"ヂ", L"ヅ", L"デ", L"ド", L"バ", L"ビ", L"ブ", L"ベ", L"ボ", L"パ", L"ピ", L"プ", L"ペ", L"ポ",
    L"ヴ", L"ヵ", L"ヶ", L"ヮ", L"ヰ", L"ヱ", L"ヂ", L"ヅ",
    
    // Hiragana (mixed in occasionally)
    L"あ", L"い", L"う", L"え", L"お", L"か", L"き", L"く", L"け", L"こ", L"さ", L"し", L"す", L"せ", L"そ",
    L"た", L"ち", L"つ", L"て", L"と", L"な", L"に", L"ぬ", L"ね", L"の", L"は", L"ひ", L"ふ", L"へ", L"ほ",
    
    // Latin letters and numbers (occasional mixing like in the movie)
    L"0", L"1", L"2", L"3", L"4", L"5", L"6", L"7", L"8", L"9",
    L"A", L"B", L"C", L"D", L"E", L"F", L"G", L"H", L"I", L"J", L"K", L"L", L"M",
    L"N", L"O", L"P", L"Q", L"R", L"S", L"T", L"U", L"V", L"W", L"X", L"Y", L"Z",
    
    // Mathematical and special symbols (rare)
    L"∑", L"∏", L"∫", L"∂", L"∆", L"∇", L"π", L"λ", L"μ", L"σ", L"φ", L"ψ", L"ω",
    L"≠", L"≤", L"≥", L"±", L"∞", L"√", L"∝", L"∈", L"∉", L"⊂", L"⊃", L"⊆", L"⊇",
    
    // Binary-looking symbols
    L"｜", L"‖", L"║", L"│", L"┃", L"┆", L"┇", L"┊", L"┋", L"╎", L"╏", L"╽", L"╿"
};

// Character categories for different effects
const std::vector<std::wstring> KATAKANA_CHARS = {
    L"ア", L"イ", L"ウ", L"エ", L"オ", L"カ", L"キ", L"ク", L"ケ", L"コ", L"サ", L"シ", L"ス", L"セ", L"ソ",
    L"タ", L"チ", L"ツ", L"テ", L"ト", L"ナ", L"ニ", L"ヌ", L"ネ", L"ノ", L"ハ", L"ヒ", L"フ", L"ヘ", L"ホ",
    L"マ", L"ミ", L"ム", L"メ", L"モ", L"ヤ", L"ユ", L"ヨ", L"ラ", L"リ", L"ル", L"レ", L"ロ", L"ワ", L"ヲ", L"ン"
};

const std::vector<std::wstring> LATIN_CHARS = {
    L"0", L"1", L"2", L"3", L"4", L"5", L"6", L"7", L"8", L"9",
    L"A", L"B", L"C", L"D", L"E", L"F", L"G", L"H", L"I", L"J", L"K", L"L", L"M",
    L"N", L"O", L"P", L"Q", L"R", L"S", L"T", L"U", L"V", L"W", L"X", L"Y", L"Z"
};

const std::vector<std::wstring> SYMBOL_CHARS = {
    L"∑", L"∏", L"∫", L"∂", L"∆", L"∇", L"π", L"λ", L"μ", L"σ", L"φ", L"ψ", L"ω",
    L"｜", L"‖", L"║", L"│", L"┃", L"┆", L"┇", L"┊", L"┋", L"╎", L"╏", L"╽", L"╿"
};

// Random number generator
extern std::mt19937 g_rng;

// Utility functions
std::wstring GetExecutablePath();
bool IsMouseMoved(const POINT& initial, const POINT& current, int threshold = 10);

// Smooth interpolation utility
inline float Lerp(float a, float b, float t) {
    return a + t * (b - a);
}