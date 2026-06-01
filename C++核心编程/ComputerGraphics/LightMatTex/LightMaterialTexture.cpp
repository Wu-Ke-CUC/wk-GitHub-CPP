#include "LightMaterialTexture.h"

#include <objidl.h>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCmd) {
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

    INITCOMMONCONTROLSEX icc { sizeof(icc), ICC_WIN95_CLASSES | ICC_BAR_CLASSES };
    InitCommonControlsEx(&icc);

    if (!RegisterWindowClasses(instance)) {
        MessageBoxW(nullptr, L"Failed to register window classes.", L"Error", MB_ICONERROR | MB_OK);
        return 1;
    }

    HWND hwnd = CreateWindowExW(
        0,
        kMainClassName,
        L"LightMaterialTexture - Win32 C++",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_MAXIMIZE,
        CW_USEDEFAULT, CW_USEDEFAULT, 1280, 980,
        nullptr, nullptr, instance, nullptr);

    if (!hwnd) {
        MessageBoxW(nullptr, L"Failed to create the main window.", L"Error", MB_ICONERROR | MB_OK);
        return 1;
    }

    ShowWindow(hwnd, SW_SHOWMAXIMIZED);
    UpdateWindow(hwnd);

    MSG msg {};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    Gdiplus::GdiplusShutdown(gdiplusToken);
    return static_cast<int>(msg.wParam);
}
