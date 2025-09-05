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
};

// Screen grid cell for persistent Matrix effect
struct GridCell {
    std::wstring character;
    float alpha = 0.0f;
    float fontSize = 14.0f;
    float depth = 0.5f;
    bool isActive = false;
    float lastUpdateTime = 0.0f;
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

// Color utilities
struct Color {
    float r, g, b, a;
    
    Color(float red = 0.0f, float green = 0.0f, float blue = 0.0f, float alpha = 1.0f)
        : r(red), g(green), b(blue), a(alpha) {}
    
    static Color FromHSV(float h, float s, float v, float a = 1.0f);
    D2D1_COLOR_F ToD2D1() const { return {r, g, b, a}; }
};

// Matrix characters - using string literals to avoid wide char constant warnings
const std::vector<std::wstring> MATRIX_CHARS = {
    L"ア", L"イ", L"ウ", L"エ", L"オ", L"カ", L"キ", L"ク", L"ケ", L"コ", L"サ", L"シ", L"ス", L"セ", L"ソ",
    L"タ", L"チ", L"ツ", L"テ", L"ト", L"ナ", L"ニ", L"ヌ", L"ネ", L"ノ", L"ハ", L"ヒ", L"フ", L"ヘ", L"ホ",
    L"マ", L"ミ", L"ム", L"メ", L"モ", L"ヤ", L"ユ", L"ヨ", L"ラ", L"リ", L"ル", L"レ", L"ロ", L"ワ", L"ヲ", L"ン",
    L"0", L"1", L"2", L"3", L"4", L"5", L"6", L"7", L"8", L"9", L"A", L"B", L"C"
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