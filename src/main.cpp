#include "common.h"
#include "matrix_screensaver.h"
#include "config_dialog.h"
#include <windowsx.h>

std::mt19937 g_rng(static_cast<unsigned int>(std::chrono::steady_clock::now().time_since_epoch().count()));

// Global variables
std::unique_ptr<MatrixScreensaver> g_screensaver;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                   PWSTR pCmdLine, int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    
    // Initialize COM
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    
    // Parse command line arguments
    std::wstring cmdLine(pCmdLine ? pCmdLine : L"");
    std::transform(cmdLine.begin(), cmdLine.end(), cmdLine.begin(), ::towlower);
    
    if (cmdLine.find(L"/c") != std::wstring::npos) {
        // Configuration mode
        ConfigDialog config;
        config.Show(hInstance);
        CoUninitialize();
        return 0;
    }
    else if (cmdLine.find(L"/p") != std::wstring::npos) {
        // Preview mode - not implemented
        CoUninitialize();
        return 0;
    }
    
    // Screensaver mode (default)
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    wc.lpszClassName = L"MatrixScreensaver";
    
    RegisterClassEx(&wc);
    
    // Create screensaver windows for each monitor
    struct MonitorData {
        HINSTANCE hInstance;
        std::vector<HWND> windows;
    };
    
    MonitorData monitorData = { hInstance };
    
    auto monitorProc = [](HMONITOR hMonitor, HDC hdcMonitor, 
                         LPRECT lprcMonitor, LPARAM dwData) -> BOOL {
        auto* data = reinterpret_cast<MonitorData*>(dwData);
        
        HWND hwnd = CreateWindowEx(
            WS_EX_TOPMOST,
            L"MatrixScreensaver",
            L"Matrix Screensaver",
            WS_POPUP | WS_VISIBLE,
            lprcMonitor->left, lprcMonitor->top,
            lprcMonitor->right - lprcMonitor->left,
            lprcMonitor->bottom - lprcMonitor->top,
            nullptr, nullptr, data->hInstance, nullptr);
        
        if (hwnd) {
            data->windows.push_back(hwnd);
            ShowWindow(hwnd, SW_SHOW);
            UpdateWindow(hwnd);
        }
        
        return TRUE;
    };
    
    EnumDisplayMonitors(nullptr, nullptr, monitorProc, 
                       reinterpret_cast<LPARAM>(&monitorData));
    
    if (monitorData.windows.empty()) {
        // Fallback to primary monitor
        HWND hwnd = CreateWindowEx(
            WS_EX_TOPMOST,
            L"MatrixScreensaver",
            L"Matrix Screensaver",
            WS_POPUP | WS_VISIBLE,
            0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
            nullptr, nullptr, hInstance, nullptr);
        
        if (hwnd) {
            monitorData.windows.push_back(hwnd);
            ShowWindow(hwnd, SW_SHOW);
            UpdateWindow(hwnd);
        }
    }
    
    // Message loop
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    CoUninitialize();
    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static POINT s_initialMousePos = { -1, -1 };
    static bool s_mouseInitialized = false;
    static std::chrono::steady_clock::time_point s_startTime;
    
    switch (uMsg) {
    case WM_CREATE:
        {
            s_startTime = std::chrono::steady_clock::now();
            
            g_screensaver = std::make_unique<MatrixScreensaver>();
            if (!g_screensaver->Initialize(hwnd)) {
                return -1;
            }
            
            // Set up timer for animation
            SetTimer(hwnd, 1, 16, nullptr); // ~60 FPS
            
            // Hide cursor
            ShowCursor(FALSE);
        }
        break;
        
    case WM_DESTROY:
        KillTimer(hwnd, 1);
        if (g_screensaver) {
            g_screensaver->Shutdown();
            g_screensaver.reset();
        }
        ShowCursor(TRUE);
        PostQuitMessage(0);
        break;
        
    case WM_TIMER:
        if (g_screensaver) {
            static auto lastFrame = std::chrono::high_resolution_clock::now();
            auto now = std::chrono::high_resolution_clock::now();
            auto deltaTime = std::chrono::duration<float>(now - lastFrame).count();
            lastFrame = now;
            
            g_screensaver->Update(deltaTime);
            g_screensaver->Render();
        }
        break;
        
    case WM_SIZE:
        if (g_screensaver) {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
            g_screensaver->Resize(width, height);
        }
        break;
        
    case WM_KEYDOWN:
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
        PostQuitMessage(0);
        break;
        
    case WM_MOUSEMOVE:
        {
            // Ignore mouse movement for the first second
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - s_startTime).count() < 1000) {
                break;
            }
            
            POINT currentPos = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            
            if (!s_mouseInitialized) {
                s_initialMousePos = currentPos;
                s_mouseInitialized = true;
            } else {
                if (IsMouseMoved(s_initialMousePos, currentPos, 10)) {
                    PostQuitMessage(0);
                }
            }
        }
        break;
        
    case WM_SYSCOMMAND:
        if (wParam == SC_SCREENSAVE || wParam == SC_MONITORPOWER) {
            return 0; // Prevent nested screensavers
        }
        break;
        
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    
    return 0;
}

// Utility function implementations
std::wstring GetExecutablePath() {
    wchar_t path[MAX_PATH];
    GetModuleFileName(nullptr, path, MAX_PATH);
    
    std::wstring fullPath(path);
    size_t lastSlash = fullPath.find_last_of(L"\\/");
    return (lastSlash != std::wstring::npos) ? fullPath.substr(0, lastSlash + 1) : fullPath;
}

bool IsMouseMoved(const POINT& initial, const POINT& current, int threshold) {
    int deltaX = abs(current.x - initial.x);
    int deltaY = abs(current.y - initial.y);
    return (deltaX > threshold || deltaY > threshold);
}

// Color utility implementation
Color Color::FromHSV(float h, float s, float v, float a) {
    float c = v * s;
    float x = c * (1.0f - abs(fmod(h * 6.0f, 2.0f) - 1.0f));
    float m = v - c;
    
    float r, g, b;
    
    if (h < 1.0f / 6.0f) {
        r = c; g = x; b = 0;
    } else if (h < 2.0f / 6.0f) {
        r = x; g = c; b = 0;
    } else if (h < 3.0f / 6.0f) {
        r = 0; g = c; b = x;
    } else if (h < 4.0f / 6.0f) {
        r = 0; g = x; b = c;
    } else if (h < 5.0f / 6.0f) {
        r = x; g = 0; b = c;
    } else {
        r = c; g = 0; b = x;
    }
    
    return Color(r + m, g + m, b + m, a);
}