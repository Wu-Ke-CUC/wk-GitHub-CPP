// ============================================================
// Hermite三次曲线 - C++ Windows x64 OpenGL + GDI+ 应用
// Visual Studio 2019 Project
// ============================================================

#include <windows.h>
#include <gdiplus.h>
#include "GLView.h"
#include "UIPanel.h"

#pragma comment(lib, "gdiplus.lib")

// ==================== 全局变量 ====================
static HINSTANCE g_hInstance = nullptr;
static ULONG_PTR g_gdiplusToken = 0;

// ==================== 布局常量 ====================
const int MAIN_PADDING = 8;
const int UI_PANEL_MIN = 480;  // UI面板最小宽度
const int GL_PANEL_MIN = 400;  // GL面板最小宽度

// 根据可用宽度自适应计算各面板尺寸，保持UI:GL ≈ 480:1000 的比例
inline void computePanelSizes(int clientW, int& glW, int& uiW) {
    int totalPad = MAIN_PADDING * 3;
    int avail = clientW - totalPad;
    uiW = max(UI_PANEL_MIN, avail * 480 / 1480);
    // 确保GL面板不低于最小值
    if (avail - uiW < GL_PANEL_MIN) uiW = max(UI_PANEL_MIN, avail - GL_PANEL_MIN);
    glW = avail - uiW;
}

// ==================== 主窗口过程 ====================
static LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    AppState* state = (AppState*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    switch (msg) {
    case WM_CREATE: {
        // 创建AppState并存储在窗口用户数据中
        state = new AppState();
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)state);

        // 初始化示例数据
        state->InitExample();

        // 获取客户区大小
        RECT rc;
        GetClientRect(hWnd, &rc);
        int cx = rc.right - rc.left;
        int cy = rc.bottom - rc.top;

        // 注册子窗口类
        RegisterGLViewClass(g_hInstance);
        RegisterUIPanelClass(g_hInstance);

        // 自适应计算面板尺寸
        int glW, uiW;
        computePanelSizes(cx, glW, uiW);
        int glH = cy - MAIN_PADDING * 2;
        CreateGLView(g_hInstance, hWnd,
            MAIN_PADDING, MAIN_PADDING, glW, glH, state);

        // 创建GDI+ UI面板（右侧）
        int uiX = MAIN_PADDING + glW + MAIN_PADDING;
        CreateUIPanel(g_hInstance, hWnd,
            uiX, MAIN_PADDING, uiW, glH, state);

        return 0;
    }

    case WM_SIZE: {
        if (!state) return 0;
        RECT rc;
        GetClientRect(hWnd, &rc);
        int cx = rc.right - rc.left;
        int cy = rc.bottom - rc.top;

        // 自适应重新布局子窗口
        int glW, uiW;
        computePanelSizes(cx, glW, uiW);
        int glH = cy - MAIN_PADDING * 2;
        if (glH < 200) glH = 200;

        if (state->hGLView) {
            SetWindowPos(state->hGLView, nullptr,
                MAIN_PADDING, MAIN_PADDING, glW, glH,
                SWP_NOZORDER | SWP_NOACTIVATE);
        }

        int uiX = MAIN_PADDING + glW + MAIN_PADDING;
        if (state->hUIPanel) {
            SetWindowPos(state->hUIPanel, nullptr,
                uiX, MAIN_PADDING, uiW, glH,
                SWP_NOZORDER | SWP_NOACTIVATE);
        }
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;  // 防止主窗口背景擦除导致闪烁

    case WM_DESTROY:
        if (state) delete state;
        PostQuitMessage(0);
        return 0;

    // 键盘快捷键：转发给GLView统一处理（GLView内部使用防闪烁守卫）
    case WM_KEYDOWN: {
        if (!state || !state->hGLView) return 0;
        if (wParam == VK_DELETE || wParam == VK_BACK) {
            SendMessage(state->hGLView, WM_DELETE_SELECTED, 0, 0);
        }
        if (wParam == VK_ESCAPE) {
            SendMessage(state->hGLView, WM_KEYDOWN, VK_ESCAPE, 0);
        }
        return 0;
    }

    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
}

// ==================== WinMain入口 ====================
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    g_hInstance = hInstance;

    // 初始化GDI+
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, nullptr);

    // 注册主窗口类
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(22, 22, 46));  // 深色背景
    wc.lpszClassName = L"HermiteMainWindow";
    RegisterClassEx(&wc);

    // 计算窗口尺寸为屏幕大小（全屏）
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    RECT desiredRect = { 0, 0, screenW, screenH };
    AdjustWindowRect(&desiredRect, WS_OVERLAPPEDWINDOW, FALSE);

    // 创建主窗口（允许最大化/调整大小）
    HWND hMainWnd = CreateWindowEx(
        0,
        L"HermiteMainWindow",
        L"Hermite 三次曲线关键帧插值 — OpenGL + GDI+",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT,
        desiredRect.right - desiredRect.left,
        desiredRect.bottom - desiredRect.top,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!hMainWnd) {
        Gdiplus::GdiplusShutdown(g_gdiplusToken);
        return -1;
    }

    ShowWindow(hMainWnd, SW_MAXIMIZE);
    UpdateWindow(hMainWnd);

    // 消息循环
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // 清理GDI+
    Gdiplus::GdiplusShutdown(g_gdiplusToken);
    return (int)msg.wParam;
}
