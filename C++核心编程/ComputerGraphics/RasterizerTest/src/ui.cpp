#include "ui.h"

#include "app_state.h"
#include "rendering.h"

#include <windowsx.h>

#include <algorithm>

namespace
{

void ApplyUiFont(HWND control)
{
    SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(g_app.uiFont), TRUE);
}

HWND CreateLabel(HWND parent, int id, const wchar_t* text, DWORD extraStyle = 0)
{
    HWND handle = CreateWindowExW(
        0,
        L"STATIC",
        text,
        WS_CHILD | WS_VISIBLE | SS_LEFT | extraStyle,
        0,
        0,
        0,
        0,
        parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        GetModuleHandleW(nullptr),
        nullptr);
    ApplyUiFont(handle);
    return handle;
}

} // namespace

void LayoutControls(HWND hwnd)
{
    RECT rc{};
    GetClientRect(hwnd, &rc);
    const int clientWidth = rc.right - rc.left;

    const int canvasX = kMargin;
    const int canvasY = kMargin;
    const int panelX = canvasX + kCanvasWidth + kMargin;
    const int panelWidth = std::max(250, std::min(kPanelWidth, clientWidth - panelX - kMargin));

    MoveWindow(g_app.canvasWindow, canvasX, canvasY, kCanvasWidth, kCanvasHeight, TRUE);

    int y = kMargin;
    const int lineHeight = 24;
    const int helpHeight = 116;
    const int statusHeight = 118;
    const int legendHeight = 200;
    const int buttonHeight = 32;

    MoveWindow(g_app.helpLabel, panelX, y, panelWidth, helpHeight, TRUE);
    y += helpHeight + 10;

    MoveWindow(g_app.startButton, panelX, y, panelWidth / 2 - 6, buttonHeight, TRUE);
    MoveWindow(g_app.resetButton, panelX + panelWidth / 2 + 6, y, panelWidth / 2 - 6, buttonHeight, TRUE);
    y += 46;

    MoveWindow(g_app.statusLabel, panelX, y, panelWidth, statusHeight, TRUE);
    y += statusHeight + 8;

    MoveWindow(g_app.hoverLabel, panelX, y, panelWidth, lineHeight, TRUE);
    y += 28;

    MoveWindow(g_app.vertexLabels[0], panelX, y, panelWidth, lineHeight, TRUE);
    y += 26;
    MoveWindow(g_app.vertexLabels[1], panelX, y, panelWidth, lineHeight, TRUE);
    y += 26;
    MoveWindow(g_app.vertexLabels[2], panelX, y, panelWidth, lineHeight, TRUE);
    y += 34;

    MoveWindow(g_app.legendLabel, panelX, y, panelWidth, legendHeight, TRUE);
}

void AddVertexFromCanvas(int x, int y)
{
    if (g_app.scanState == ScanState::FlashingBoundingBox || g_app.scanState == ScanState::Scanning)
    {
        return;
    }

    if (g_app.vertices.size() >= 3)
    {
        g_app.statusText = L"当前三角形已定义完成，请点击“重置”后重新选择顶点。";
        UpdateStatus();
        return;
    }

    g_app.vertices.push_back({x, y});
    UpdateVertexLabels();

    if (g_app.vertices.size() < 3)
    {
        wchar_t buffer[128];
        std::swprintf(buffer, 128, L"已设置顶点 %d: (%d, %d)，继续点击下一个顶点。", static_cast<int>(g_app.vertices.size()), x, y);
        g_app.statusText = buffer;
        g_app.scanState = ScanState::Idle;
    }
    else
    {
        RebuildTriangleData();
        UpdateVertexLabels();
        g_app.scanState = ScanState::Ready;
        g_app.statusText =
            L"三角形已创建。当前程序保留了顶点输入与轮廓显示；扫描转换部分留作练习，"
            L"请继续完成 rasterizer_algorithm.cpp 中的 6 个核心函数。";
    }

    UpdateStatus();
    RequestCanvasRedraw();
}

bool CreateMainUi(HWND hwnd)
{
    g_app.canvasWindow = CreateWindowExW(
        0,
        L"RasterCanvasWindow",
        L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER,
        0,
        0,
        kCanvasWidth,
        kCanvasHeight,
        hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kIdCanvas)),
        GetModuleHandleW(nullptr),
        nullptr);

    if (g_app.canvasWindow == nullptr)
    {
        return false;
    }

    g_app.helpLabel = CreateLabel(
        hwnd,
        kIdHelpLabel,
        L"已完成内容:\r\n1. 在左侧网格中点击 3 个格子输入顶点。\r\n2. 输入 3 个顶点后显示三角形轮廓。\r\n3. 鼠标悬停可查看当前格点坐标。\r\n4. 点击“重置”按钮可重新开始。");

    g_app.startButton = CreateWindowExW(
        0,
        L"BUTTON",
        L"开始扫描(待实现)",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        0,
        0,
        0,
        0,
        hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kIdStartButton)),
        GetModuleHandleW(nullptr),
        nullptr);

    g_app.resetButton = CreateWindowExW(
        0,
        L"BUTTON",
        L"重置",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        0,
        0,
        0,
        0,
        hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kIdResetButton)),
        GetModuleHandleW(nullptr),
        nullptr);

    g_app.statusLabel = CreateLabel(hwnd, kIdStatusLabel, L"");
    g_app.hoverLabel = CreateLabel(hwnd, kIdHoverLabel, L"当前位置: (移出画布)");
    g_app.vertexLabels[0] = CreateLabel(hwnd, kIdVertex1Label, L"顶点1: 未设置");
    g_app.vertexLabels[1] = CreateLabel(hwnd, kIdVertex2Label, L"顶点2: 未设置");
    g_app.vertexLabels[2] = CreateLabel(hwnd, kIdVertex3Label, L"顶点3: 未设置");
    g_app.legendLabel = CreateLabel(
        hwnd,
        kIdLegendLabel,
        L"待完成函数:\r\n1. TriangleSignedArea2\r\n2. BuildEdgeEquations\r\n3. IsDegenerateTriangle\r\n4. CalculateBoundingBox\r\n5. EvaluateInside\r\n6. InterpolateColor\r\n\r\n完成这些函数后，再恢复扫描转换与填充演示。");

    for (HWND control : {g_app.startButton, g_app.resetButton, g_app.statusLabel, g_app.hoverLabel,
                         g_app.vertexLabels[0], g_app.vertexLabels[1], g_app.vertexLabels[2], g_app.helpLabel, g_app.legendLabel})
    {
        ApplyUiFont(control);
    }

    UpdateStatus();
    UpdateVertexLabels();
    UpdateHoverLabel();
    return true;
}

LRESULT CALLBACK CanvasWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_ERASEBKGND:
        return 1;

    case WM_LBUTTONDOWN:
    {
        const int mouseX = GET_X_LPARAM(lParam);
        const int mouseY = GET_Y_LPARAM(lParam);
        const int gridX = std::clamp(mouseX / kCellSize, 0, kGridCols - 1);
        const int gridY = std::clamp(mouseY / kCellSize, 0, kGridRows - 1);
        AddVertexFromCanvas(gridX, gridY);
        return 0;
    }

    case WM_MOUSEMOVE:
    {
        const int mouseX = GET_X_LPARAM(lParam);
        const int mouseY = GET_Y_LPARAM(lParam);
        g_app.hoverX = std::clamp(mouseX / kCellSize, 0, kGridCols - 1);
        g_app.hoverY = std::clamp(mouseY / kCellSize, 0, kGridRows - 1);
        g_app.hoverValid = true;
        UpdateHoverLabel();
        RequestCanvasRedraw();

        TRACKMOUSEEVENT tme{};
        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = hwnd;
        TrackMouseEvent(&tme);
        return 0;
    }

    case WM_MOUSELEAVE:
        g_app.hoverValid = false;
        UpdateHoverLabel();
        RequestCanvasRedraw();
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps{};
        BeginPaint(hwnd, &ps);
        if (g_app.glRc != nullptr)
        {
            wglMakeCurrent(g_app.glDc, g_app.glRc);
            RenderScene();
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        g_app.mainWindow = hwnd;
        NONCLIENTMETRICSW metrics{};
        metrics.cbSize = sizeof(metrics);
        SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0);
        g_app.uiFont = CreateFontIndirectW(&metrics.lfMessageFont);

        if (!CreateMainUi(hwnd))
        {
            return -1;
        }

        if (!InitializeOpenGL(g_app.canvasWindow))
        {
            MessageBoxW(hwnd, L"OpenGL 初始化失败。", L"错误", MB_ICONERROR | MB_OK);
            return -1;
        }

        LayoutControls(hwnd);
        return 0;
    }

    case WM_SIZE:
        LayoutControls(hwnd);
        return 0;

    case WM_COMMAND:
    {
        const int controlId = LOWORD(wParam);
        const int notifyCode = HIWORD(wParam);
        if (controlId == kIdStartButton && notifyCode == BN_CLICKED)
        {
            StartScanning();
            return 0;
        }
        if (controlId == kIdResetButton && notifyCode == BN_CLICKED)
        {
            ResetAll();
            return 0;
        }
        return 0;
    }

    case WM_TIMER:
        if (wParam == kAnimationTimerId)
        {
            if (g_app.scanState == ScanState::FlashingBoundingBox)
            {
                ++g_app.flashTick;
                g_app.flashAltColor = !g_app.flashAltColor;
                if (g_app.flashTick >= 6)
                {
                    g_app.scanState = ScanState::Scanning;
                    g_app.statusText = L"步骤 3-5: 正在按包围盒逐格扫描，显示边函数判定并对内部像素执行插值填充。";
                    UpdateStatus();
                    SetTimer(g_app.mainWindow, kAnimationTimerId, kScanMs, nullptr);
                }
                RequestCanvasRedraw();
                return 0;
            }
            if (g_app.scanState == ScanState::Scanning)
            {
                AdvanceOneScanStep();
                return 0;
            }
        }
        return 0;

    case WM_DESTROY:
        KillTimer(hwnd, kAnimationTimerId);
        DestroyOpenGL();
        if (g_app.uiFont != nullptr)
        {
            DeleteObject(g_app.uiFont);
            g_app.uiFont = nullptr;
        }
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
