#pragma once

#include <windows.h>

bool CreateMainUi(HWND hwnd);
void LayoutControls(HWND hwnd);
void AddVertexFromCanvas(int x, int y);

LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK CanvasWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
