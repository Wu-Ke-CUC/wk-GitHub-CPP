#include <windows.h>

#include "app_state.h"
#include "ui.h"

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand)
{
    const wchar_t kMainClassName[] = L"TriangleRasterizerMainWindow";
    const wchar_t kCanvasClassName[] = L"RasterCanvasWindow";

    WNDCLASSEXW mainClass{};
    mainClass.cbSize = sizeof(mainClass);
    mainClass.style = CS_HREDRAW | CS_VREDRAW;
    mainClass.lpfnWndProc = MainWindowProc;
    mainClass.hInstance = instance;
    mainClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    mainClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    mainClass.lpszClassName = kMainClassName;

    WNDCLASSEXW canvasClass{};
    canvasClass.cbSize = sizeof(canvasClass);
    canvasClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    canvasClass.lpfnWndProc = CanvasWindowProc;
    canvasClass.hInstance = instance;
    canvasClass.hCursor = LoadCursorW(nullptr, IDC_CROSS);
    canvasClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    canvasClass.lpszClassName = kCanvasClassName;

    if (!RegisterClassExW(&mainClass) || !RegisterClassExW(&canvasClass))
    {
        return 0;
    }

    const DWORD style = WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX;
    RECT desired = {0, 0, kMargin * 3 + kCanvasWidth + kPanelWidth, kMargin * 2 + kCanvasHeight + 16};
    AdjustWindowRect(&desired, style, FALSE);

    HWND hwnd = CreateWindowExW(
        0,
        kMainClassName,
        L"GPU 三角形扫描转换模拟 - Win32 + OpenGL",
        style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        desired.right - desired.left,
        desired.bottom - desired.top,
        nullptr,
        nullptr,
        instance,
        nullptr);

    if (hwnd == nullptr)
    {
        return 0;
    }

    ShowWindow(hwnd, showCommand);
    UpdateWindow(hwnd);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
}
