#include "UIPanel.h"
#include <windowsx.h>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

// ==================== 布局常量 ====================
const int PANEL_PADDING = 12;
const int SECTION_SPACING = 14;
const int BUTTON_HEIGHT = 38;
const int BUTTON_SPACING = 8;
const int SLIDER_HEIGHT = 28;
const int LABEL_HEIGHT = 22;
const int SECTION_HEADER_HEIGHT = 32;

// ==================== UI控件区域结构 ====================
struct ButtonRect {
    RECT rc;
    const WCHAR* text;
};
struct SliderRect {
    RECT track;     // 轨道区域
    const WCHAR* label;
    int minVal, maxVal;
    int* valuePtr;
    WCHAR fmtBuffer[64];
};
struct SectionHeader {
    RECT rc;
    const WCHAR* text;
};
struct RadioItem {
    RECT hitArea;       // 点击区域
    RECT indicator;     // 圆形指示器
    const WCHAR* text;
    bool selected;
};

// ==================== GDI+绘制辅助函数 ====================

static void drawDarkBackground(Graphics& g, const RECT& rc) {
    // 深色背景
    SolidBrush bgBrush(Color(255, 26, 26, 46));  // #1a1a2e
    g.FillRectangle(&bgBrush, Rect(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top));

    // 顶部微高光
    Color topColor(255, 40, 40, 65);
    Color bottomColor(255, 22, 22, 50);
    LinearGradientBrush gradient(
        Point(0, rc.top), Point(0, rc.top + 60),
        topColor, bottomColor);
    g.FillRectangle(&gradient, Rect(rc.left, rc.top, rc.right - rc.left, 60));
}

static void drawSectionPanel(Graphics& g, const RECT& rc) {
    // 半透明面板
    SolidBrush panelBrush(Color(15, 255, 255, 255));
    Pen borderPen(Color(25, 255, 255, 255));

    GraphicsPath path;
    path.AddArc(rc.left, rc.top, 20, 20, 180, 90);
    path.AddArc(rc.right - 20, rc.top, 20, 20, 270, 90);
    path.AddArc(rc.right - 20, rc.bottom - 20, 20, 20, 0, 90);
    path.AddArc(rc.left, rc.bottom - 20, 20, 20, 90, 90);
    path.CloseFigure();

    g.FillPath(&panelBrush, &path);
    g.DrawPath(&borderPen, &path);
}

static void drawSectionHeader(Graphics& g, const SectionHeader& header, Font& font) {
    SolidBrush textBrush(Color(255, 0, 212, 255));  // #00d4ff
    PointF origin((float)header.rc.left, (float)header.rc.top + 8);
    g.DrawString(header.text, -1, &font, origin, &textBrush);

    // 下划线
    Pen linePen(Color(25, 255, 255, 255));
    g.DrawLine(&linePen,
        (float)header.rc.left, (float)header.rc.bottom - 1,
        (float)header.rc.right, (float)header.rc.bottom - 1);
}

static void drawRadioGroup(Graphics& g, RadioItem* radios, int count, Font& font) {
    for (int i = 0; i < count; i++) {
        auto& radio = radios[i];
        int cx = radio.indicator.left + (radio.indicator.right - radio.indicator.left) / 2;
        int cy = radio.indicator.top + (radio.indicator.bottom - radio.indicator.top) / 2;
        int r = 7;

        // 外圈
        Color outerColor = radio.selected
            ? Color(255, 0, 212, 255)    // 选中: 青色
            : Color(255, 100, 100, 110); // 未选中: 灰色
        Pen outerPen(outerColor, 2.0f);
        g.DrawEllipse(&outerPen, cx - r, cy - r, r * 2, r * 2);

        // 选中内圆点
        if (radio.selected) {
            SolidBrush innerBrush(Color(255, 0, 212, 255));
            g.FillEllipse(&innerBrush, cx - 3, cy - 3, 6, 6);
        }

        // 文字
        SolidBrush textBrush(Color(255, 220, 220, 220));
        PointF textPt((float)radio.indicator.right + 6, (float)cy - 8);
        g.DrawString(radio.text, -1, &font, textPt, &textBrush);
    }
}

static void drawButton(Graphics& g, const RECT& rc, const WCHAR* text,
    Font& font, Color fillTop, Color fillBottom, bool hovered)
{
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;

    // 按钮背景渐变
    GraphicsPath path;
    path.AddArc(rc.left, rc.top, 10, 10, 180, 90);
    path.AddArc(rc.right - 10, rc.top, 10, 10, 270, 90);
    path.AddArc(rc.right - 10, rc.bottom - 10, 10, 10, 0, 90);
    path.AddArc(rc.left, rc.bottom - 10, 10, 10, 90, 90);
    path.CloseFigure();

    LinearGradientBrush brush(
        Point(0, rc.top), Point(0, rc.bottom),
        fillTop, fillBottom);
    g.FillPath(&brush, &path);

    // 悬停效果
    if (hovered) {
        Color hoverColor(30, 255, 255, 255);
        SolidBrush hoverBrush(hoverColor);
        g.FillPath(&hoverBrush, &path);
    }

    // 文字居中
    SolidBrush textBrush(Color(255, 255, 255, 255));
    StringFormat fmt;
    fmt.SetAlignment(StringAlignmentCenter);
    fmt.SetLineAlignment(StringAlignmentCenter);
    RectF rf((float)rc.left, (float)rc.top, (float)w, (float)h);
    g.DrawString(text, -1, &font, rf, &fmt, &textBrush);
}

static void drawSlider(Graphics& g, SliderRect& slider,
    Font& labelFont, Font& valueFont)
{
    if (!slider.valuePtr) return;

    int w = slider.track.right - slider.track.left;
    int trackY = slider.track.top + (slider.track.bottom - slider.track.top) / 2;

    // 标签
    SolidBrush labelBrush(Color(255, 170, 170, 170));
    PointF labelPt((float)slider.track.left, (float)slider.track.top - 22);
    g.DrawString(slider.label, -1, &labelFont, labelPt, &labelBrush);

    // 轨道背景
    SolidBrush trackBg(Color(255, 40, 40, 55));
    g.FillRectangle(&trackBg, Rect(slider.track.left, trackY - 3, w, 6));

    // 填充部分
    int value = *slider.valuePtr;
    float ratio = (float)(value - slider.minVal) / (slider.maxVal - slider.minVal);
    if (ratio < 0) ratio = 0;
    if (ratio > 1) ratio = 1;
    int fillWidth = (int)(w * ratio);

    SolidBrush trackFill(Color(255, 0, 212, 255));  // #00d4ff
    if (fillWidth > 0) {
        g.FillRectangle(&trackFill, Rect(slider.track.left, trackY - 3, fillWidth, 6));
    }

    // 滑块圆形
    int thumbX = slider.track.left + fillWidth;
    SolidBrush thumbBrush(Color(255, 0, 212, 255));
    g.FillEllipse(&thumbBrush, thumbX - 7, trackY - 7, 14, 14);
    Pen thumbBorder(Color(255, 255, 255, 255));
    g.DrawEllipse(&thumbBorder, thumbX - 7, trackY - 7, 14, 14);

    // 数值
    SolidBrush valueBrush(Color(255, 200, 200, 200));
    StringFormat rightFmt;
    rightFmt.SetAlignment(StringAlignmentFar);
    RectF valueRect((float)slider.track.right - 60, (float)slider.track.top - 22, 60, 20);
    swprintf_s(slider.fmtBuffer, 64, L"%.2f", ratio * (slider.maxVal - slider.minVal) / 100.0f);
    g.DrawString(slider.fmtBuffer, -1, &valueFont, valueRect, &rightFmt, &valueBrush);
}

static void drawStatusBar(Graphics& g, const RECT& rc, Font& font,
    const WCHAR* text, int statusType)
{
    // 背景面板
    SolidBrush panelBrush(Color(50, 0, 0, 0));
    GraphicsPath path;
    path.AddArc(rc.left, rc.top, 10, 10, 180, 90);
    path.AddArc(rc.right - 10, rc.top, 10, 10, 270, 90);
    path.AddArc(rc.right - 10, rc.bottom - 10, 10, 10, 0, 90);
    path.AddArc(rc.left, rc.bottom - 10, 10, 10, 90, 90);
    path.CloseFigure();
    g.FillPath(&panelBrush, &path);

    // 状态文字颜色
    Color textColor;
    switch (statusType) {
    case 1:  textColor = Color(255, 255, 193, 7);   break; // #ffc107 - editing pos
    case 2:  textColor = Color(255, 255, 107, 107); break; // #ff6b6b - editing tangent
    case 3:  textColor = Color(255, 255, 193, 7);   break; // #ffc107 - adding
    default: textColor = Color(255, 107, 255, 107); break; // #6bff6b - normal
    }

    SolidBrush textBrush(textColor);
    RectF textRect((float)rc.left + 10, (float)rc.top + 6,
        (float)(rc.right - rc.left - 20), (float)(rc.bottom - rc.top - 12));
    StringFormat fmt;
    g.DrawString(text, -1, &font, textRect, &fmt, &textBrush);
}

static void drawLegend(Graphics& g, const RECT& rc, Font& font) {
    struct LegendItem { Color color; const WCHAR* text; };
    LegendItem items[] = {
        { Color(255, 0, 212, 255),   L"插值曲线" },
        { Color(255, 255, 107, 107), L"关键帧" },
        { Color(255, 107, 255, 107), L"默认切向量" },
        { Color(255, 255, 193, 7),   L"选中/编辑" },
        { Color(255, 255, 215, 0),   L"运动物体" },
    };

    float y = (float)rc.top;
    StringFormat fmt;
    fmt.SetLineAlignment(StringAlignmentCenter);

    for (auto& item : items) {
        // 色块
        SolidBrush colorBrush(item.color);
        g.FillRectangle(&colorBrush, (REAL)rc.left, y + 8.0f, 20.0f, 3.0f);

        // 文字
        SolidBrush textBrush(Color(255, 180, 180, 180));
        RectF textRect((REAL)rc.left + 28.0f, y, (REAL)(rc.right - rc.left - 28), 20.0f);
        g.DrawString(item.text, -1, &font, textRect, &fmt, &textBrush);

        y += 20.0f;
    }
}

static void drawInfoBox(Graphics& g, const RECT& rc, Font& headerFont, Font& bodyFont) {
    SolidBrush panelBrush(Color(12, 255, 255, 255));
    Pen borderPen(Color(20, 255, 255, 255));

    GraphicsPath path;
    path.AddArc(rc.left, rc.top, 10, 10, 180, 90);
    path.AddArc(rc.right - 10, rc.top, 10, 10, 270, 90);
    path.AddArc(rc.right - 10, rc.bottom - 10, 10, 10, 0, 90);
    path.AddArc(rc.left, rc.bottom - 10, 10, 10, 90, 90);
    path.CloseFigure();
    g.FillPath(&panelBrush, &path);
    g.DrawPath(&borderPen, &path);

    // 标题
    SolidBrush titleBrush(Color(255, 0, 212, 255));
    PointF titlePt((float)rc.left + 10, (float)rc.top + 8);
    g.DrawString(L"Hermite插值说明", -1, &headerFont, titlePt, &titleBrush);

    // 说明文字
    SolidBrush bodyBrush(Color(255, 170, 170, 170));
    const WCHAR* instructions[] = {
        L"* Hermite三次样条曲线关键帧插值",
        L"* 基函数: ",
        L"           h00=2t³-3t²+1",
        L"           h10=t³-2t²+t",
        L"           h01=-2t³+3t²",
        L"           h11=t³-t²",
        L"",
        L"* 交互: ",
        L"  点击关键帧->拖拽移动->抬起鼠标后调切向量->点击确认",
        L"  点击空白处新增关键帧，抬起鼠标后调切向量->点击确认",
        L"  Del或删除键可删选中关键帧 / Esc取消"
    };
    float y = (float)rc.top + 34;
    for (auto* line : instructions) {
        PointF pt((float)rc.left + 10, y);
        g.DrawString(line, -1, &bodyFont, pt, &bodyBrush);
        y += 18.0f;
    }
}

// ==================== UI面板窗口过程 ====================
static LRESULT CALLBACK UIPanelWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    HWND hParent = GetParent(hWnd);
    AppState* state = (AppState*)GetWindowLongPtr(hParent, GWLP_USERDATA);

    // 静态变量存储控件位置（在WM_SIZE中计算）
    static ButtonRect btnPlayPause, btnReset, btnDelete, btnClear;
    static SliderRect sliderTime, sliderSpeed;
    static RadioItem radioTrajectory[2];
    static SectionHeader headerAnim, headerInfo;
    static RECT statusRect, legendRect, infoRect, sectionAnimRect, sectionInfoRect;
    static int panelWidth = 0, panelHeight = 0;
    static bool btnHover[4] = { false, false, false, false };
    static bool sliderTimeDragging = false;
    static bool sliderSpeedDragging = false;

    switch (msg) {
    case WM_CREATE: {
        // 启用鼠标离开追踪
        TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, hWnd, 0 };
        TrackMouseEvent(&tme);

        // 将状态指针存储到窗口
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)state);
        return 0;
    }

    case WM_SIZE: {
        RECT rc;
        GetClientRect(hWnd, &rc);
        panelWidth = rc.right - rc.left;
        panelHeight = rc.bottom - rc.top;

        int cx = panelWidth;
        int y = PANEL_PADDING;

        // ===== Section 1: 动画控制 =====
        int sec1Top = y;
        sectionAnimRect = { PANEL_PADDING, y, cx - PANEL_PADDING, y };
        y += SECTION_HEADER_HEIGHT;

        // 进度滑块标签区
        y += LABEL_HEIGHT + 4;
        sliderTime.label = L"动画进度:";
        sliderTime.track = { PANEL_PADDING + 10, y, cx - PANEL_PADDING - 10, y + SLIDER_HEIGHT };
        sliderTime.minVal = 0;
        sliderTime.maxVal = 1000;
        sliderTime.valuePtr = (state ? &state->sliderPos : nullptr);
        y += SLIDER_HEIGHT + 6;

        // 速度滑块
        y += 4;
        sliderSpeed.label = L"动画速度:";
        sliderSpeed.track = { PANEL_PADDING + 10, y, cx - PANEL_PADDING - 10, y + SLIDER_HEIGHT };
        sliderSpeed.minVal = 1;
        sliderSpeed.maxVal = 500;
        sliderSpeed.valuePtr = (state ? &state->speedSliderPos : nullptr);
        y += SLIDER_HEIGHT + 10;

        // ===== 轨迹模式 RadioBox =====
        int radioH = 22;
        int halfW = (cx - PANEL_PADDING * 2 - 20) / 2;
        radioTrajectory[0].hitArea = { PANEL_PADDING + 10, y, PANEL_PADDING + 10 + halfW, y + radioH };
        radioTrajectory[0].indicator = { PANEL_PADDING + 10, y + 3, PANEL_PADDING + 24, y + radioH - 3 };
        radioTrajectory[0].text = L"折线段轨迹";
        radioTrajectory[0].selected = (state ? state->trajectoryMode == 0 : true);

        radioTrajectory[1].hitArea = { PANEL_PADDING + 10 + halfW, y, cx - PANEL_PADDING - 10, y + radioH };
        radioTrajectory[1].indicator = { PANEL_PADDING + 10 + halfW, y + 3, PANEL_PADDING + 24 + halfW, y + radioH - 3 };
        radioTrajectory[1].text = L"Hermite曲线";
        radioTrajectory[1].selected = (state ? state->trajectoryMode == 1 : false);
        y += radioH + 10;

        // 按钮区
        auto makeBtnRect = [&](const WCHAR* text) -> ButtonRect {
            ButtonRect br;
            br.text = text;
            br.rc = { PANEL_PADDING + 10, y, cx - PANEL_PADDING - 10, y + BUTTON_HEIGHT };
            y += BUTTON_HEIGHT + BUTTON_SPACING;
            return br;
        };
        btnPlayPause = makeBtnRect(L"播放");
        btnReset = makeBtnRect(L"重置动画");
        btnDelete = makeBtnRect(L"删除选中关键帧");
        btnClear = makeBtnRect(L"清空所有关键帧");

        sectionAnimRect.bottom = y;
        int sec1Bottom = y;

        y += SECTION_SPACING;

        // ===== Section 2: 作业说明 =====
        infoRect = { PANEL_PADDING, y, cx - PANEL_PADDING, y };
        int infoTop = y;
        y += 250;  // 预估信息框高度
        infoRect.bottom = y;

        y += SECTION_SPACING;

        // ===== 状态栏 =====
        statusRect = { PANEL_PADDING, y, cx - PANEL_PADDING, y + 50 };
        y += 60;

        // ===== 图例 =====
        legendRect = { PANEL_PADDING, y, cx - PANEL_PADDING, y + 100 };
        y += 120;

        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rc;
        GetClientRect(hWnd, &rc);
        int w = rc.right - rc.left;
        int h = rc.bottom - rc.top;

        // ===== 双缓冲：先绘制到内存DC，再一次BitBlt到屏幕 =====
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBmp = CreateCompatibleBitmap(hdc, w, h);
        HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

        // 创建GDI+ Graphics（绘制到内存DC）
        Graphics g(memDC);
        g.SetSmoothingMode(SmoothingModeAntiAlias);
        g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

        // 背景
        drawDarkBackground(g, rc);

        // Fonts
        Font headerFont(L"Segoe UI", 12, FontStyleBold);
        Font bodyFont(L"Segoe UI", 10, FontStyleRegular);
        Font sliderLabelFont(L"Segoe UI", 9, FontStyleRegular);
        Font buttonFont(L"Segoe UI", 10, FontStyleRegular);
        Font smallFont(L"Segoe UI", 8, FontStyleRegular);

        // ===== Section 1: 动画控制 =====
        drawSectionPanel(g, sectionAnimRect);
        SectionHeader sh1 = { { sectionAnimRect.left + 8, sectionAnimRect.top,
            sectionAnimRect.right - 8, sectionAnimRect.top + SECTION_HEADER_HEIGHT },
            L"动画控制" };
        drawSectionHeader(g, sh1, headerFont);

        // 滑块
        drawSlider(g, sliderTime, sliderLabelFont, smallFont);
        drawSlider(g, sliderSpeed, sliderLabelFont, smallFont);

        // ===== 轨迹模式 RadioBox =====
        drawRadioGroup(g, radioTrajectory, 2, sliderLabelFont);

        // 按钮
        // 播放/暂停 - 蓝色
        Color playTop(0, 212, 255), playBottom(0, 153, 204);
        const WCHAR* playText = (state && state->isPlaying) ? L"暂停" : L"播放";
        drawButton(g, btnPlayPause.rc, playText, buttonFont,
            playTop, playBottom, btnHover[0]);

        // 重置 - 灰色
        Color resetTop(80, 80, 90), resetBottom(50, 50, 60);
        drawButton(g, btnReset.rc, L"重置动画", buttonFont,
            resetTop, resetBottom, btnHover[1]);

        // 删除 - 黄色
        Color delTop(255, 193, 7), delBottom(0xE0, 0xA8, 0);
        drawButton(g, btnDelete.rc, L"删除选中关键帧", buttonFont,
            delTop, delBottom, btnHover[2]);

        // 清空 - 红色
        Color clearTop(255, 107, 107), clearBottom(0xEE, 0x5A, 0x5A);
        drawButton(g, btnClear.rc, L"清空所有关键帧", buttonFont,
            clearTop, clearBottom, btnHover[3]);

        // ===== Section 2: 信息 =====
        drawInfoBox(g, infoRect, headerFont, bodyFont);

        // ===== 状态栏 =====
        if (state) {
            drawStatusBar(g, statusRect, bodyFont, state->statusText, state->statusType);
        }

        // ===== 图例 =====
        drawLegend(g, legendRect, smallFont);

        // 一次性将完整画面拷贝到屏幕（消除中间绘制过程可见的闪烁）
        BitBlt(hdc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);

        // 清理双缓冲资源
        SelectObject(memDC, oldBmp);
        DeleteObject(memBmp);
        DeleteDC(memDC);

        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;  // 防止闪烁

    case WM_MOUSEMOVE: {
        if (!state) return 0;
        int mx = GET_X_LPARAM(lParam);
        int my = GET_Y_LPARAM(lParam);

        // 检查按钮悬停
        ButtonRect* buttons[4] = { &btnPlayPause, &btnReset, &btnDelete, &btnClear };
        bool needsRedraw = false;
        for (int i = 0; i < 4; i++) {
            bool inside = PtInRect(&buttons[i]->rc, { mx, my });
            if (inside != btnHover[i]) {
                btnHover[i] = inside;
                needsRedraw = true;
            }
        }

        // 滑块拖动
        if (sliderTimeDragging) {
            int relX = mx - sliderTime.track.left;
            int tw = sliderTime.track.right - sliderTime.track.left;
            if (tw > 0) {
                float ratio = (float)relX / tw;
                if (ratio < 0) ratio = 0;
                if (ratio > 1) ratio = 1;
                int val = (int)(ratio * (sliderTime.maxVal - sliderTime.minVal));
                if (state) state->sliderPos = val;
                SendMessage(state->hGLView, WM_SET_ANIM_TIME, val, 0);
            }
        }
        if (sliderSpeedDragging) {
            int relX = mx - sliderSpeed.track.left;
            int tw = sliderSpeed.track.right - sliderSpeed.track.left;
            if (tw > 0) {
                float ratio = (float)relX / tw;
                if (ratio < 0) ratio = 0;
                if (ratio > 1) ratio = 1;
                int val = (int)(sliderSpeed.minVal + ratio * (sliderSpeed.maxVal - sliderSpeed.minVal));
                if (state) state->speedSliderPos = val;
                SendMessage(state->hGLView, WM_SET_ANIM_SPEED, val, 0);
            }
        }

        if (needsRedraw) InvalidateRect(hWnd, nullptr, FALSE);

        // 刷新鼠标离开追踪
        TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, hWnd, 0 };
        TrackMouseEvent(&tme);
        return 0;
    }

    case WM_LBUTTONDOWN: {
        int mx = GET_X_LPARAM(lParam);
        int my = GET_Y_LPARAM(lParam);
        POINT pt = { mx, my };

        // RadioBox 点击
        for (int i = 0; i < 2; i++) {
            if (PtInRect(&radioTrajectory[i].hitArea, pt)) {
                radioTrajectory[0].selected = (i == 0);
                radioTrajectory[1].selected = (i == 1);
                if (state) {
                    state->trajectoryMode = i;
                    SendMessage(state->hGLView, WM_SET_TRAJECTORY, i, 0);
                }
                InvalidateRect(hWnd, nullptr, FALSE);
                return 0;
            }
        }

        // 滑条按下
        if (PtInRect(&sliderTime.track, pt)) {
            sliderTimeDragging = true;
            SetCapture(hWnd);
            // 立即更新位置
            int relX = mx - sliderTime.track.left;
            int tw = sliderTime.track.right - sliderTime.track.left;
            if (tw > 0) {
                float ratio = (float)relX / tw;
                if (ratio < 0) ratio = 0;
                if (ratio > 1) ratio = 1;
                int val = (int)(ratio * (sliderTime.maxVal - sliderTime.minVal));
                if (state) state->sliderPos = val;
                SendMessage(state->hGLView, WM_SET_ANIM_TIME, val, 0);
            }
            InvalidateRect(hWnd, nullptr, FALSE);
            return 0;
        }
        if (PtInRect(&sliderSpeed.track, pt)) {
            sliderSpeedDragging = true;
            SetCapture(hWnd);
            int relX = mx - sliderSpeed.track.left;
            int tw = sliderSpeed.track.right - sliderSpeed.track.left;
            if (tw > 0) {
                float ratio = (float)relX / tw;
                if (ratio < 0) ratio = 0;
                if (ratio > 1) ratio = 1;
                int val = (int)(sliderSpeed.minVal + ratio * (sliderSpeed.maxVal - sliderSpeed.minVal));
                if (state) state->speedSliderPos = val;
                SendMessage(state->hGLView, WM_SET_ANIM_SPEED, val, 0);
            }
            InvalidateRect(hWnd, nullptr, FALSE);
            return 0;
        }
        return 0;
    }

    case WM_LBUTTONUP: {
        int mx = GET_X_LPARAM(lParam);
        int my = GET_Y_LPARAM(lParam);
        POINT pt = { mx, my };

        // 释放滑条捕获
        if (sliderTimeDragging) {
            sliderTimeDragging = false;
            ReleaseCapture();
            return 0;
        }
        if (sliderSpeedDragging) {
            sliderSpeedDragging = false;
            ReleaseCapture();
            return 0;
        }

        // 按钮点击检测
        if (!state) return 0;
        ButtonRect* buttons[4] = { &btnPlayPause, &btnReset, &btnDelete, &btnClear };
        UINT msgs[4] = { WM_PLAYBACK_TOGGLE, WM_RESET_ANIM, WM_DELETE_SELECTED, WM_CLEAR_ALL };

        for (int i = 0; i < 4; i++) {
            if (PtInRect(&buttons[i]->rc, pt)) {
                SendMessage(state->hGLView, msgs[i], 0, 0);
                return 0;
            }
        }
        return 0;
    }

    case WM_MOUSELEAVE: {
        for (int i = 0; i < 4; i++) btnHover[i] = false;
        InvalidateRect(hWnd, nullptr, FALSE);
        return 0;
    }

    case WM_DESTROY:
        return 0;

    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
}

// ==================== 注册与创建 ====================

void RegisterUIPanelClass(HINSTANCE hInstance) {
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_DBLCLKS;  // 不需要CS_HREDRAW/CS_VREDRAW避免不必要重绘
    wc.lpfnWndProc = UIPanelWndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = L"GDIUIPanel";
    RegisterClassEx(&wc);
}

HWND CreateUIPanel(HINSTANCE hInstance, HWND hParent, int x, int y, int w, int h, AppState* state) {
    HWND hWnd = CreateWindowEx(
        0, L"GDIUIPanel", nullptr,
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
        x, y, w, h,
        hParent, nullptr, hInstance, nullptr
    );

    if (hWnd) {
        state->hUIPanel = hWnd;
    }
    return hWnd;
}
