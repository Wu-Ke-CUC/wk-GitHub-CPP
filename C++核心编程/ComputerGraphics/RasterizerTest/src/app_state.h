#pragma once

#include <windows.h>

#include <string>
#include <vector>

inline constexpr int kGridCols = 40;
inline constexpr int kGridRows = 30;
inline constexpr int kCellSize = 20;
inline constexpr int kCanvasWidth = kGridCols * kCellSize;
inline constexpr int kCanvasHeight = kGridRows * kCellSize;
inline constexpr int kPanelWidth = 320;
inline constexpr int kMargin = 16;

inline constexpr UINT_PTR kAnimationTimerId = 1;
inline constexpr UINT kBoundingBoxFlashMs = 250;
inline constexpr UINT kScanMs = 30;

inline constexpr int kIdCanvas = 100;
inline constexpr int kIdStartButton = 101;
inline constexpr int kIdResetButton = 102;
inline constexpr int kIdStatusLabel = 104;
inline constexpr int kIdHoverLabel = 105;
inline constexpr int kIdVertex1Label = 106;
inline constexpr int kIdVertex2Label = 107;
inline constexpr int kIdVertex3Label = 108;
inline constexpr int kIdHelpLabel = 109;
inline constexpr int kIdLegendLabel = 110;

struct IntPoint
{
    int x = 0;
    int y = 0;
};

struct FloatColor
{
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
};

struct EdgeEquation
{
    float a = 0.0f;
    float b = 0.0f;
    float c = 0.0f;

    float Evaluate(float x, float y) const
    {
        return a * x + b * y + c;
    }
};

struct BoundingBox
{
    int minX = 0;
    int maxX = 0;
    int minY = 0;
    int maxY = 0;
    bool valid = false;
};

struct CellState
{
    bool visited = false;
    bool inside = false;
    float edgeValues[3]{};
    FloatColor color{};
};

enum class ScanState
{
    Idle,
    Ready,
    FlashingBoundingBox,
    Scanning,
    Done
};

struct AppState
{
    HWND mainWindow = nullptr;
    HWND canvasWindow = nullptr;
    HWND startButton = nullptr;
    HWND resetButton = nullptr;
    HWND statusLabel = nullptr;
    HWND hoverLabel = nullptr;
    HWND vertexLabels[3]{};
    HWND helpLabel = nullptr;
    HWND legendLabel = nullptr;
    HFONT uiFont = nullptr;

    HDC glDc = nullptr;
    HGLRC glRc = nullptr;

    ScanState scanState = ScanState::Idle;

    std::vector<IntPoint> vertices;
    IntPoint triangle[3]{};
    EdgeEquation edges[3]{};
    BoundingBox boundingBox{};
    bool triangleReady = false;
    bool triangleDegenerate = false;

    std::vector<CellState> scannedCells = std::vector<CellState>(kGridCols * kGridRows);

    int currentScanX = 0;
    int currentScanY = 0;
    int flashTick = 0;
    bool flashAltColor = false;

    bool hoverValid = false;
    int hoverX = 0;
    int hoverY = 0;

    std::wstring statusText = L"请在画布上依次点击 3 个格子定义三角形顶点。扫描转换核心函数已留空，等待后续实现。";

    const FloatColor vertexColors[3] = {
        FloatColor{1.0f, 0.0f, 0.0f},
        FloatColor{0.0f, 1.0f, 0.0f},
        FloatColor{0.0f, 0.0f, 1.0f},
    };
};

extern AppState g_app;

int CellIndex(int x, int y);
float Clamp01(float value);
FloatColor MakeColor(float r, float g, float b);
void SetControlText(HWND handle, const std::wstring& text);
std::wstring FormatPointText(const wchar_t* prefix, const IntPoint& p);
void UpdateVertexLabels();
void UpdateHoverLabel();
void UpdateStatus();
void RequestCanvasRedraw();
void RebuildTriangleData();
void ResetScanResults();
void ResetAll();
void AdvanceOneScanStep();
void StartScanning();
