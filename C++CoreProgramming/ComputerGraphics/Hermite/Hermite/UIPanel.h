#pragma once

#include <windows.h>
#include "GLView.h"

// 注册GDI+ UI面板子窗口类
void RegisterUIPanelClass(HINSTANCE hInstance);

// 创建GDI+ UI面板子窗口
HWND CreateUIPanel(HINSTANCE hInstance, HWND hParent, int x, int y, int w, int h, AppState* state);
