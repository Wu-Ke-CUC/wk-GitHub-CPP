#include "rendering.h"

#include "app_state.h"
#include "rasterizer_algorithm.h"

#include <gl/GL.h>

#include <cmath>

#pragma comment(lib, "opengl32.lib")

// 抗锯齿开关：1 = 开启4x SSAA，0 = 关闭（原始离散填充）
#define ENABLE_ANTIALIASING 1

namespace
{

FloatColor BackgroundColor()
{
    return MakeColor(0.97f, 0.98f, 0.99f);
}

void DrawRectFilled(float x, float y, float w, float h, const FloatColor& color)
{
    glColor3f(color.r, color.g, color.b);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

void DrawRectOutline(float x, float y, float w, float h, const FloatColor& color, float lineWidth)
{
    glLineWidth(lineWidth);
    glColor3f(color.r, color.g, color.b);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

void DrawLine(float x0, float y0, float x1, float y1, const FloatColor& color, float lineWidth, bool dashed)
{
    glLineWidth(lineWidth);
    if (dashed)
    {
        glEnable(GL_LINE_STIPPLE);
        glLineStipple(1, 0x00FF);
    }
    else
    {
        glDisable(GL_LINE_STIPPLE);
    }

    glColor3f(color.r, color.g, color.b);
    glBegin(GL_LINES);
    glVertex2f(x0, y0);
    glVertex2f(x1, y1);
    glEnd();

    glDisable(GL_LINE_STIPPLE);
}

void DrawCircle(float cx, float cy, float radius, const FloatColor& fill, const FloatColor& stroke, float strokeWidth)
{
    constexpr int kSegments = 32;

    glColor3f(fill.r, fill.g, fill.b);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= kSegments; ++i)
    {
        const float angle = static_cast<float>(i) / static_cast<float>(kSegments) * 6.28318530718f;
        glVertex2f(cx + std::cos(angle) * radius, cy + std::sin(angle) * radius);
    }
    glEnd();

    glLineWidth(strokeWidth);
    glColor3f(stroke.r, stroke.g, stroke.b);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < kSegments; ++i)
    {
        const float angle = static_cast<float>(i) / static_cast<float>(kSegments) * 6.28318530718f;
        glVertex2f(cx + std::cos(angle) * radius, cy + std::sin(angle) * radius);
    }
    glEnd();
}

void DrawGrid()
{
    DrawRectFilled(0.0f, 0.0f, static_cast<float>(kCanvasWidth), static_cast<float>(kCanvasHeight), BackgroundColor());

    const FloatColor lineColor = MakeColor(0.88f, 0.90f, 0.93f);
    glLineWidth(1.0f);
    glColor3f(lineColor.r, lineColor.g, lineColor.b);
    glBegin(GL_LINES);
    for (int x = 0; x <= kCanvasWidth; x += kCellSize)
    {
        glVertex2f(static_cast<float>(x), 0.0f);
        glVertex2f(static_cast<float>(x), static_cast<float>(kCanvasHeight));
    }
    for (int y = 0; y <= kCanvasHeight; y += kCellSize)
    {
        glVertex2f(0.0f, static_cast<float>(y));
        glVertex2f(static_cast<float>(kCanvasWidth), static_cast<float>(y));
    }
    glEnd();
}

void DrawVertexMarker(const IntPoint& v, const FloatColor& color)
{
    const float px = v.x * kCellSize + kCellSize * 0.5f;
    const float py = v.y * kCellSize + kCellSize * 0.5f;
    DrawCircle(px, py, kCellSize * 0.35f, color, MakeColor(1.0f, 1.0f, 1.0f), 2.0f);
}

void DrawTriangleOutline()
{
    if (!g_app.triangleReady)
    {
        return;
    }

    const FloatColor outline = MakeColor(1.0f, 0.80f, 0.20f);
    for (int i = 0; i < 3; ++i)
    {
        const IntPoint& a = g_app.triangle[i];
        const IntPoint& b = g_app.triangle[(i + 1) % 3];
        DrawLine(
            a.x * kCellSize + kCellSize * 0.5f,
            a.y * kCellSize + kCellSize * 0.5f,
            b.x * kCellSize + kCellSize * 0.5f,
            b.y * kCellSize + kCellSize * 0.5f,
            outline,
            2.0f,
            true);
    }
}

void DrawVertices()
{
    for (int i = 0; i < static_cast<int>(g_app.vertices.size()) && i < 3; ++i)
    {
        DrawVertexMarker(g_app.vertices[i], g_app.vertexColors[i]);
    }
}

void DrawTestDots(int x, int y, const float edgeValues[3], bool inside)
{
    const float cellX = static_cast<float>(x) * kCellSize;
    const float cellY = static_cast<float>(y) * kCellSize;
    const float cx = cellX + kCellSize * 0.5f;
    const float cy = cellY + kCellSize * 0.5f;
    const float radius = kCellSize * 0.11f;
    const float orbit = kCellSize * 0.28f;
    const float angles[3] = {
        -3.1415926535f * 0.5f,
        3.1415926535f / 6.0f,
        5.0f * 3.1415926535f / 6.0f,
    };

    const FloatColor positive = MakeColor(1.0f, 0.42f, 0.42f);
    const FloatColor negative = MakeColor(0.29f, 0.56f, 0.89f);

    for (int i = 0; i < 3; ++i)
    {
        const float dotX = cx + std::cos(angles[i]) * orbit;
        const float dotY = cy + std::sin(angles[i]) * orbit;
        const FloatColor fill = edgeValues[i] >= 0.0f ? positive : negative;
        DrawCircle(dotX, dotY, radius, fill, MakeColor(1.0f, 1.0f, 1.0f), 1.0f);
    }

    if (inside)
    {
        DrawRectOutline(cellX + 2.0f, cellY + 2.0f, static_cast<float>(kCellSize - 4), static_cast<float>(kCellSize - 4), MakeColor(0.18f, 0.80f, 0.45f), 2.0f);
    }
}

void DrawFinalRaster()
{
    // 背景色（用于采样点在三角形外的颜色）
    const FloatColor bgColor = BackgroundColor();

#if ENABLE_ANTIALIASING
    // 4x SSAA：每个像素分割为 2x2 个子采样点
    const int ssaaLevel = 2;                // 2x2 网格
    const float subStep = 1.0f / ssaaLevel; // 子采样步长 0.5
    const float subOffset = subStep * 0.5f; // 偏移量 0.25，使采样点位于子像素中心

    for (int y = 0; y < kGridRows; ++y)
    {
        for (int x = 0; x < kGridCols; ++x)
        {
            FloatColor accumColor = { 0.0f, 0.0f, 0.0f };
            int samplesInside = 0;

            // 遍历子采样点
            for (int sy = 0; sy < ssaaLevel; ++sy)
            {
                for (int sx = 0; sx < ssaaLevel; ++sx)
                {
                    // 子采样点的连续坐标（格子坐标范围，例如 x + 0.25, x + 0.75）
                    float px = static_cast<float>(x) + subOffset + static_cast<float>(sx) * subStep;
                    float py = static_cast<float>(y) + subOffset + static_cast<float>(sy) * subStep;

                    if (EvaluateInside(g_app.edges, px, py))
                    {
                        FloatColor sampleColor = InterpolateColor(g_app.triangle, g_app.vertexColors, px, py);
                        accumColor.r += sampleColor.r;
                        accumColor.g += sampleColor.g;
                        accumColor.b += sampleColor.b;
                        ++samplesInside;
                    }
                    else
                    {
                        // 使用背景色
                        accumColor.r += bgColor.r;
                        accumColor.g += bgColor.g;
                        accumColor.b += bgColor.b;
                    }
                }
            }

            const float invTotal = 1.0f / (ssaaLevel * ssaaLevel);
            FloatColor finalColor = {
                Clamp01(accumColor.r * invTotal),
                Clamp01(accumColor.g * invTotal),
                Clamp01(accumColor.b * invTotal)
            };

            // 只有当至少有一个采样点在三角形内时，才填充该格子（否则完全透明/背景）
            if (samplesInside > 0)
            {
                DrawRectFilled(
                    static_cast<float>(x * kCellSize + 1),
                    static_cast<float>(y * kCellSize + 1),
                    static_cast<float>(kCellSize - 2),
                    static_cast<float>(kCellSize - 2),
                    finalColor);
            }
        }
    }
#else
    // 原始离散填充（无抗锯齿）
    for (int y = 0; y < kGridRows; ++y)
    {
        for (int x = 0; x < kGridCols; ++x)
        {
            // 使用格子中心点（x+0.5, y+0.5）判断，与原逻辑一致
            if (!EvaluateInside(g_app.edges, static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f))
            {
                continue;
            }

            const FloatColor color = InterpolateColor(g_app.triangle, g_app.vertexColors,
                static_cast<float>(x) + 0.5f,
                static_cast<float>(y) + 0.5f);
            DrawRectFilled(
                static_cast<float>(x * kCellSize + 1),
                static_cast<float>(y * kCellSize + 1),
                static_cast<float>(kCellSize - 2),
                static_cast<float>(kCellSize - 2),
                color);
        }
    }
#endif
}

void DrawBoundingBox()
{
    if (!g_app.boundingBox.valid)
    {
        return;
    }

    FloatColor color = MakeColor(0.95f, 0.61f, 0.14f);
    bool dashed = false;
    if (g_app.scanState == ScanState::FlashingBoundingBox)
    {
        color = g_app.flashAltColor ? MakeColor(0.91f, 0.30f, 0.24f) : MakeColor(0.95f, 0.61f, 0.14f);
        dashed = true;
    }

    DrawRectOutline(
        static_cast<float>(g_app.boundingBox.minX * kCellSize),
        static_cast<float>(g_app.boundingBox.minY * kCellSize),
        static_cast<float>((g_app.boundingBox.maxX - g_app.boundingBox.minX + 1) * kCellSize),
        static_cast<float>((g_app.boundingBox.maxY - g_app.boundingBox.minY + 1) * kCellSize),
        color,
        2.5f);

    if (dashed)
    {
        const float x = static_cast<float>(g_app.boundingBox.minX * kCellSize);
        const float y = static_cast<float>(g_app.boundingBox.minY * kCellSize);
        const float w = static_cast<float>((g_app.boundingBox.maxX - g_app.boundingBox.minX + 1) * kCellSize);
        const float h = static_cast<float>((g_app.boundingBox.maxY - g_app.boundingBox.minY + 1) * kCellSize);
        DrawLine(x, y, x + w, y, color, 2.0f, true);
        DrawLine(x + w, y, x + w, y + h, color, 2.0f, true);
        DrawLine(x + w, y + h, x, y + h, color, 2.0f, true);
        DrawLine(x, y + h, x, y, color, 2.0f, true);
    }
}

void DrawCurrentScanHighlight()
{
    if (g_app.scanState != ScanState::Scanning || !g_app.boundingBox.valid)
    {
        return;
    }

    if (g_app.currentScanY > g_app.boundingBox.maxY)
    {
        return;
    }

    DrawRectOutline(
        static_cast<float>(g_app.currentScanX * kCellSize + 1),
        static_cast<float>(g_app.currentScanY * kCellSize + 1),
        static_cast<float>(kCellSize - 2),
        static_cast<float>(kCellSize - 2),
        MakeColor(0.26f, 0.60f, 0.88f),
        2.0f);
}

void DrawHoverHighlight()
{
    if (!g_app.hoverValid || g_app.scanState == ScanState::Scanning)
    {
        return;
    }

    DrawRectOutline(
        static_cast<float>(g_app.hoverX * kCellSize + 1),
        static_cast<float>(g_app.hoverY * kCellSize + 1),
        static_cast<float>(kCellSize - 2),
        static_cast<float>(kCellSize - 2),
        MakeColor(0.45f, 0.55f, 0.72f),
        1.5f);
}

} // namespace

void RenderScene()
{
    RECT rc{};
    GetClientRect(g_app.canvasWindow, &rc);
    const int width = rc.right - rc.left;
    const int height = rc.bottom - rc.top;

    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, static_cast<double>(width), static_cast<double>(height), 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glClearColor(0.97f, 0.98f, 0.99f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    DrawGrid();

    if (g_app.scanState == ScanState::Done)
    {
        DrawFinalRaster();
    }
    else if (g_app.scanState == ScanState::Scanning)
    {
        for (int y = g_app.boundingBox.minY; y <= g_app.boundingBox.maxY; ++y)
        {
            for (int x = g_app.boundingBox.minX; x <= g_app.boundingBox.maxX; ++x)
            {
                const CellState& cell = g_app.scannedCells[CellIndex(x, y)];
                if (!cell.visited)
                {
                    continue;
                }

                // During scanning, keep the edge-function dots visible for every tested cell.
                // Cells that pass the inside test are only highlighted; the final fill appears
                // after the full bounding box has been processed.
                DrawTestDots(x, y, cell.edgeValues, cell.inside);
            }
        }
    }

    if (g_app.scanState == ScanState::Ready || g_app.scanState == ScanState::FlashingBoundingBox)
    {
        DrawTriangleOutline();
    }

    if (g_app.scanState == ScanState::FlashingBoundingBox || g_app.scanState == ScanState::Scanning || g_app.scanState == ScanState::Done)
    {
        DrawBoundingBox();
    }

    if (g_app.scanState == ScanState::Done)
    {
        DrawTriangleOutline();
    }

    DrawVertices();
    DrawHoverHighlight();
    DrawCurrentScanHighlight();

    SwapBuffers(g_app.glDc);
}

bool InitializeOpenGL(HWND hwnd)
{
    g_app.glDc = GetDC(hwnd);
    if (g_app.glDc == nullptr)
    {
        return false;
    }

    PIXELFORMATDESCRIPTOR pfd{};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.iLayerType = PFD_MAIN_PLANE;

    const int pixelFormat = ChoosePixelFormat(g_app.glDc, &pfd);
    if (pixelFormat == 0 || !SetPixelFormat(g_app.glDc, pixelFormat, &pfd))
    {
        return false;
    }

    g_app.glRc = wglCreateContext(g_app.glDc);
    if (g_app.glRc == nullptr)
    {
        return false;
    }

    if (!wglMakeCurrent(g_app.glDc, g_app.glRc))
    {
        return false;
    }

    return true;
}

void DestroyOpenGL()
{
    if (g_app.glRc != nullptr)
    {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(g_app.glRc);
        g_app.glRc = nullptr;
    }

    if (g_app.canvasWindow != nullptr && g_app.glDc != nullptr)
    {
        ReleaseDC(g_app.canvasWindow, g_app.glDc);
        g_app.glDc = nullptr;
    }
}
