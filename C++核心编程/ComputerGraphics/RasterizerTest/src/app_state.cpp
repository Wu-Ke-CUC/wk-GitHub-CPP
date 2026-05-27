#include "app_state.h"
#include "rasterizer_algorithm.h" 
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
        g_app.triangle[i] = g_app.vertices[i];

    // 构建边方程
    BuildEdgeEquations(g_app.triangle, g_app.edges);

    // 计算包围盒
    g_app.boundingBox = CalculateBoundingBox(g_app.triangle);

    // 检测退化
    g_app.triangleDegenerate = IsDegenerateTriangle(g_app.triangle);

    g_app.triangleReady = !g_app.triangleDegenerate;
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
    // 仅在扫描状态下执行
    if (g_app.scanState != ScanState::Scanning)
        return;

    // 检查是否已完成所有格子
    if (g_app.currentScanY > g_app.boundingBox.maxY)
    {
        // 扫描完成
        KillTimer(g_app.mainWindow, kAnimationTimerId);
        g_app.scanState = ScanState::Done;
        g_app.statusText = L"扫描转换完成！三角形内部区域已用插值颜色填充。";
        UpdateStatus();
        RequestCanvasRedraw();
        return;
    }

    // 获取当前扫描的格子坐标
    int x = g_app.currentScanX;
    int y = g_app.currentScanY;
    int index = CellIndex(x, y);

    // 计算该格子是否在三角形内，并获取三个边方程的值
    float edgeVals[3];
    bool inside = EvaluateInside(g_app.edges, static_cast<float>(x), static_cast<float>(y), edgeVals);

    // 存储扫描结果
    g_app.scannedCells[index].visited = true;
    g_app.scannedCells[index].inside = inside;
    for (int i = 0; i < 3; ++i)
        g_app.scannedCells[index].edgeValues[i] = edgeVals[i];

    // 移动到下一个格子 (行优先)
    g_app.currentScanX++;
    if (g_app.currentScanX > g_app.boundingBox.maxX)
    {
        g_app.currentScanX = g_app.boundingBox.minX;
        g_app.currentScanY++;
    }

    // 更新状态文本（每10步更新一次，避免过于频繁）
    static int stepCount = 0;
    if (++stepCount >= 10)
    {
        stepCount = 0;
        wchar_t buffer[128];
        std::swprintf(buffer, 128, L"扫描中: 处理格子 (%d, %d)，包围盒进度 %.1f%%",
            x, y,
            100.0f * (g_app.currentScanY - g_app.boundingBox.minY) / (g_app.boundingBox.maxY - g_app.boundingBox.minY + 1));
        g_app.statusText = buffer;
        UpdateStatus();
    }

    // 刷新画布显示当前格子的判定结果
    RequestCanvasRedraw();
}

void StartScanning()
{
    if (!g_app.triangleReady)
    {
        g_app.statusText = L"请先在左侧网格中点击 3 个格子，形成有效的非退化三角形后再开始扫描。";
        UpdateStatus();
        return;
    }

    if (g_app.triangleDegenerate)
    {
        g_app.statusText = L"当前三角形退化（面积过小），请重置后重新选择顶点。";
        UpdateStatus();
        return;
    }

    // 重置扫描结果数组
    ResetScanResults();

    // 初始化扫描位置为包围盒左上角
    g_app.currentScanX = g_app.boundingBox.minX;
    g_app.currentScanY = g_app.boundingBox.minY;

    // 重置闪烁状态
    g_app.flashTick = 0;
    g_app.flashAltColor = false;

    // 进入闪烁包围盒阶段
    g_app.scanState = ScanState::FlashingBoundingBox;
    g_app.statusText = L"步骤 1-2: 高亮显示三角形包围盒，准备扫描转换...";
    UpdateStatus();

    // 启动定时器用于闪烁动画 (每 150ms 触发一次)
    SetTimer(g_app.mainWindow, kAnimationTimerId, 150, nullptr);
    RequestCanvasRedraw();
}
