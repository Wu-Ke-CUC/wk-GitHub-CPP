#include "app_state.h"

#include <cwchar>

AppState g_app;

int CellIndex(int x, int y)
{
    return y * kGridCols + x;
}

float Clamp01(float value)
{
    return std::max(0.0f, std::min(1.0f, value));
}

FloatColor MakeColor(float r, float g, float b)
{
    return FloatColor{Clamp01(r), Clamp01(g), Clamp01(b)};
}

void SetControlText(HWND handle, const std::wstring& text)
{
    SetWindowTextW(handle, text.c_str());
}

std::wstring FormatPointText(const wchar_t* prefix, const IntPoint& p)
{
    wchar_t buffer[64];
    std::swprintf(buffer, 64, L"%ls(%d, %d)", prefix, p.x, p.y);
    return buffer;
}

void UpdateVertexLabels()
{
    for (int i = 0; i < 3; ++i)
    {
        if (i < static_cast<int>(g_app.vertices.size()))
        {
            SetControlText(g_app.vertexLabels[i], FormatPointText((std::wstring(L"顶点") + std::to_wstring(i + 1) + L": ").c_str(), g_app.vertices[i]));
        }
        else
        {
            wchar_t buffer[64];
            std::swprintf(buffer, 64, L"顶点%d: 未设置", i + 1);
            SetControlText(g_app.vertexLabels[i], buffer);
        }
    }
}

void UpdateHoverLabel()
{
    if (g_app.hoverValid)
    {
        wchar_t buffer[64];
        std::swprintf(buffer, 64, L"当前位置: (%d, %d)", g_app.hoverX, g_app.hoverY);
        SetControlText(g_app.hoverLabel, buffer);
    }
    else
    {
        SetControlText(g_app.hoverLabel, L"当前位置: (移出画布)");
    }
}

void UpdateStatus()
{
    SetControlText(g_app.statusLabel, g_app.statusText);
}

void RequestCanvasRedraw()
{
    if (g_app.canvasWindow != nullptr)
    {
        InvalidateRect(g_app.canvasWindow, nullptr, FALSE);
    }
}

void RebuildTriangleData()
{
    g_app.triangleReady = false;
    g_app.triangleDegenerate = false;
    g_app.boundingBox = {};

    if (g_app.vertices.size() != 3)
    {
        return;
    }

    for (int i = 0; i < 3; ++i)
    {
        g_app.triangle[i] = g_app.vertices[i];
        g_app.edges[i] = {};
    }

    g_app.triangleReady = true;
}

void ResetScanResults()
{
    for (CellState& cell : g_app.scannedCells)
    {
        cell = {};
    }
}

void ResetAll()
{
    KillTimer(g_app.mainWindow, kAnimationTimerId);

    g_app.scanState = ScanState::Idle;
    g_app.vertices.clear();
    for (int i = 0; i < 3; ++i)
    {
        g_app.triangle[i] = {};
        g_app.edges[i] = {};
    }
    g_app.boundingBox = {};
    g_app.triangleReady = false;
    g_app.triangleDegenerate = false;
    g_app.currentScanX = 0;
    g_app.currentScanY = 0;
    g_app.flashTick = 0;
    g_app.flashAltColor = false;
    ResetScanResults();

    g_app.statusText = L"请在画布上依次点击 3 个格子定义三角形顶点。扫描转换核心函数已留空，等待后续实现。";
    UpdateStatus();
    UpdateVertexLabels();
    RequestCanvasRedraw();
}

void AdvanceOneScanStep()
{
    KillTimer(g_app.mainWindow, kAnimationTimerId);
    g_app.scanState = ScanState::Ready;
    g_app.statusText = L"扫描动画尚未启用。请先完成 rasterizer_algorithm.cpp 中的核心函数实现。";
    UpdateStatus();
    RequestCanvasRedraw();
}

void StartScanning()
{
    if (!g_app.triangleReady)
    {
        g_app.statusText = L"请先在左侧网格中点击 3 个格子，形成三角形后再开始扫描。";
        UpdateStatus();
        return;
    }

    UpdateStatus();
    RequestCanvasRedraw();
}
