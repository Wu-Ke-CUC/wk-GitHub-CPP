#include "GLView.h"
#include <windowsx.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#include <cstdio>

#define TIMER_ANIM 1
#define ANIM_TIMER_INTERVAL 16  // ~60fps

// ==================== OpenGL绘制辅助函数 ====================

// 画填充圆
static void drawFilledCircle(float cx, float cy, float radius, int segments = 32) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= segments; i++) {
        float angle = (float)(2.0 * M_PI * i / segments);
        glVertex2f(cx + cosf(angle) * radius, cy + sinf(angle) * radius);
    }
    glEnd();
}

// 画空心圆
static void drawCircle(float cx, float cy, float radius, int segments = 32) {
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segments; i++) {
        float angle = (float)(2.0 * M_PI * i / segments);
        glVertex2f(cx + cosf(angle) * radius, cy + sinf(angle) * radius);
    }
    glEnd();
}

// 画箭头
static void drawArrow(float fromX, float fromY, float toX, float toY, float headSize = 15.0f) {
    glBegin(GL_LINES);
    glVertex2f(fromX, fromY);
    glVertex2f(toX, toY);
    glEnd();

    float dx = toX - fromX, dy = toY - fromY;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.001f) return;
    float ux = dx / len, uy = dy / len;
    float angle = atan2f(uy, ux);

    float a1 = angle - (float)M_PI / 6.0f;
    float a2 = angle + (float)M_PI / 6.0f;

    glBegin(GL_TRIANGLES);
    glVertex2f(toX, toY);
    glVertex2f(toX - headSize * cosf(a1), toY - headSize * sinf(a1));
    glVertex2f(toX - headSize * cosf(a2), toY - headSize * sinf(a2));
    glEnd();
}

// 设置颜色(RGB 0~1)
static void setColor(float r, float g, float b, float a = 1.0f) {
    glColor4f(r, g, b, a);
}

// OpenGL 文字渲染（基于 wglUseFontBitmaps 创建的显示列表）
static GLuint g_fontBase = 0;

static void renderText(float x, float y, const char* text) {
    if (!g_fontBase) return;
    glRasterPos2f(x, y);
    glListBase(g_fontBase);
    glCallLists((GLsizei)strlen(text), GL_UNSIGNED_BYTE, text);
}

// ==================== 渲染函数 ====================

static void renderGrid(int width, int height) {
    setColor(1.0f, 1.0f, 1.0f, 0.08f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    for (int x = 0; x <= width; x += 50) {
        glVertex2i(x, 0);
        glVertex2i(x, height);
    }
    for (int y = 0; y <= height; y += 50) {
        glVertex2i(0, y);
        glVertex2i(width, y);
    }
    glEnd();
}

static void renderCurve(const std::vector<Point2f>& points) {
    if (points.size() < 2) return;
    setColor(0.0f, 0.83f, 1.0f, 1.0f); // #00d4ff
    glLineWidth(3.0f);
    glEnable(GL_LINE_SMOOTH);
    glBegin(GL_LINE_STRIP);
    for (const auto& p : points) {
        glVertex2f(p.x, p.y);
    }
    glEnd();
    glDisable(GL_LINE_SMOOTH);
}

// 折线段渲染（关键帧之间的直线连接）
static void renderPolyline(const std::vector<Keyframe>& keyframes) {
    if (keyframes.size() < 2) return;
    setColor(1.0f, 0.6f, 0.2f, 0.8f); // 橙色虚线风格
    glLineWidth(2.5f);
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(1, 0x0F0F);  // 虚线
    glBegin(GL_LINE_STRIP);
    for (const auto& kf : keyframes) {
        glVertex2f(kf.x, kf.y);
    }
    glEnd();
    glDisable(GL_LINE_STIPPLE);
}

static void renderKeyframes(const std::vector<Keyframe>& keyframes,
    EditStep editStep, int editingIndex, int selectedIndex)
{
    for (size_t i = 0; i < keyframes.size(); i++) {
        const Keyframe& kf = keyframes[i];

        // 选中时绘制25px命中圈（半透明）
        if (kf.selected) {
            setColor(1.0f, 0.76f, 0.03f, 0.2f);  // #ffc107 at 20%
            drawFilledCircle(kf.x, kf.y, 25.0f, 48);
        }

        Point2f arrowEnd = kf.GetArrowEnd();

        // 箭头颜色：切向量编辑=红色，选中=黄色，默认=绿色
        if (editStep == EditStep::EditingTangent && (int)i == editingIndex) {
            setColor(1.0f, 0.42f, 0.42f, 1.0f);  // #ff6b6b
        } else if (kf.selected) {
            setColor(1.0f, 0.76f, 0.03f, 1.0f);  // #ffc107
        } else {
            setColor(0.42f, 1.0f, 0.42f, 1.0f);  // #6bff6b
        }
        glLineWidth(2.5f);
        drawArrow(kf.x, kf.y, arrowEnd.x, arrowEnd.y);

        // 关键帧本体
        if (kf.selected) {
            setColor(1.0f, 0.84f, 0.0f, 1.0f); // #ffd700
            drawFilledCircle(kf.x, kf.y, 12.0f);
            setColor(1.0f, 1.0f, 1.0f, 1.0f);
            glLineWidth(2.0f);
            drawCircle(kf.x, kf.y, 12.0f);
        } else {
            setColor(1.0f, 0.42f, 0.42f, 1.0f); // #ff6b6b
            drawFilledCircle(kf.x, kf.y, 10.0f);
            setColor(1.0f, 1.0f, 1.0f, 1.0f);
            glLineWidth(2.0f);
            drawCircle(kf.x, kf.y, 10.0f);
        }
    }
}

static void renderPendingKeyframe(const Keyframe* pending) {
    if (!pending) return;

    Point2f arrowEnd = pending->GetArrowEnd();
    setColor(1.0f, 0.76f, 0.03f, 1.0f);  // #ffc107
    glLineWidth(2.5f);
    drawArrow(pending->x, pending->y, arrowEnd.x, arrowEnd.y);

    setColor(1.0f, 0.6f, 0.0f, 1.0f);  // #ff9800
    drawFilledCircle(pending->x, pending->y, 12.0f);
}

// 渲染沿路径运动的动画物体（金色光晕圆点）
// 折线段模式（trajectoryMode=0）已可用；Hermite模式需学生完成算法后生效
static void renderAnimObject(const std::vector<Keyframe>& keyframes, float animTime, int trajectoryMode) {
    if (keyframes.empty()) return;
    Point2f pos = getAnimPositionByMode(keyframes, animTime, trajectoryMode);

    // 外圈光晕
    setColor(1.0f, 0.84f, 0.0f, 0.3f);
    drawFilledCircle(pos.x, pos.y, 18.0f, 36);

    // 实体
    setColor(1.0f, 0.84f, 0.0f, 1.0f);
    drawFilledCircle(pos.x, pos.y, 10.0f);

    setColor(1.0f, 1.0f, 1.0f, 1.0f);
    glLineWidth(2.0f);
    drawCircle(pos.x, pos.y, 10.0f);
}

static void renderTimeline(int width, int height, float animTime) {
    float timelineY = (float)height - 30.0f;
    float timelineStart = 20.0f;
    float timelineEnd = (float)width - 20.0f;
    float indicatorX = timelineStart + animTime * (timelineEnd - timelineStart);

    // 时间轴线
    setColor(1.0f, 1.0f, 1.0f, 0.3f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    glVertex2f(timelineStart, timelineY);
    glVertex2f(timelineEnd, timelineY);
    glEnd();

    // 动画位置指示器
    setColor(1.0f, 0.84f, 0.0f, 1.0f);
    drawFilledCircle(indicatorX, timelineY, 8.0f);
    setColor(1.0f, 1.0f, 1.0f, 1.0f);
    glLineWidth(1.5f);
    drawCircle(indicatorX, timelineY, 8.0f);
}

// 时间轴上绘制关键帧标记点
static void renderTimelineKeyframes(const std::vector<Keyframe>& keyframes, int width, int height) {
    float timelineY = (float)height - 30.0f;
    float timelineStart = 20.0f;
    float timelineEnd = (float)width - 20.0f;

    setColor(1.0f, 0.42f, 0.42f, 0.8f);  // #ff6b6b
    for (const auto& kf : keyframes) {
        float kx = timelineStart + kf.time * (timelineEnd - timelineStart);
        drawFilledCircle(kx, timelineY, 4.0f, 12);
    }
}

// 时间轴刻度线和数字标签
static void renderTimelineTicks(int width, int height) {
    float lineY = (float)height - 30.0f;
    float startX = 20.0f;
    float endX = (float)width - 20.0f;
    float span = endX - startX;

    glLineWidth(1.0f);

    // 20等分刻度 (0.0 ~ 1.0, 步长0.05)
    for (int i = 0; i <= 20; i++) {
        float t = i / 20.0f;
        float x = startX + t * span;
        bool isMajor = (i % 2 == 0);   // 0.0, 0.1, 0.2... 为主刻度
        float tickH = isMajor ? 10.0f : 5.0f;

        // 刻度线
        setColor(1.0f, 1.0f, 1.0f, isMajor ? 0.35f : 0.18f);
        glBegin(GL_LINES);
        glVertex2f(x, lineY - tickH);
        glVertex2f(x, lineY + tickH);
        glEnd();

        // 主刻度标注数字
        if (isMajor) {
            setColor(1.0f, 1.0f, 1.0f, 0.45f);
            char buf[8];
            snprintf(buf, sizeof(buf), "%.1f", t);
            // 文字居中（每个字符约7px宽）
            float textW = strlen(buf) * 7.0f;
            renderText(x - textW * 0.5f, lineY + 14.0f, buf);
        }
    }
}

// ==================== OpenGL全局渲染 ====================
static void renderScene(AppState* state, int width, int height) {
    glClearColor(0.051f, 0.067f, 0.090f, 1.0f);  // #0d1117
    glClear(GL_COLOR_BUFFER_BIT);

    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1, 1);  // 左上角为原点，与像素坐标一致
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 1. 网格
    renderGrid(width, height);

    // 2. 曲线/折线段（根据轨迹模式）
    if (state->trajectoryMode == 1) {
        renderCurve(state->curvePoints);
    } else {
        renderPolyline(state->keyframes);
    }

    // 3. 已有关键帧
    renderKeyframes(state->keyframes, state->editStep, state->editingIndex, state->selectedIndex);

    // 4. 新增关键帧预览
    if (state->hasPendingKeyframe) {
        renderPendingKeyframe(&state->pendingKeyframe);
    }

    // 5. 动画物体
    renderAnimObject(state->keyframes, state->animationTime, state->trajectoryMode);

    // 6. 时间轴
    renderTimelineTicks(width, height);
    renderTimeline(width, height, state->animationTime);
    renderTimelineKeyframes(state->keyframes, width, height);

    glDisable(GL_BLEND);
    glFlush();
}

// ==================== 坐标转换 ====================
// 从屏幕坐标转换为OpenGL画布坐标（考虑窗口与客户区的偏移）
static Point2f screenToGLCoords(LPARAM lParam) {
    POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
    // 客户区坐标已经正确，无需额外转换
    return Point2f((float)pt.x, (float)pt.y);
}

// ==================== 防闪烁：仅在状态变化时刷新UI面板 ====================
static void InvalidateUIPanelIfChanged(AppState* state) {
    if (!state || !state->hUIPanel) return;

    bool changed = false;

    // 检查状态文本/类型变化
    if (wcscmp(state->statusText, state->lastPanelText) != 0 ||
        state->statusType != state->lastPanelType) {
        wcscpy_s(state->lastPanelText, state->statusText);
        state->lastPanelType = state->statusType;
        changed = true;
    }

    // 检查滑块数值变化
    if (state->sliderPos != state->lastSliderPos) {
        state->lastSliderPos = state->sliderPos;
        changed = true;
    }
    if (state->speedSliderPos != state->lastSpeedSliderPos) {
        state->lastSpeedSliderPos = state->speedSliderPos;
        changed = true;
    }

    // 检查轨迹模式变化
    if (state->trajectoryMode != state->lastTrajectoryMode) {
        state->lastTrajectoryMode = state->trajectoryMode;
        changed = true;
    }

    if (changed) {
        InvalidateRect(state->hUIPanel, nullptr, FALSE);
    }
}

// 检测鼠标是否悬停在某个关键帧上
static int hitTestKeyframes(const std::vector<Keyframe>& keyframes, float mx, float my) {
    for (int i = (int)keyframes.size() - 1; i >= 0; i--) {
        if (keyframes[i].ContainsPoint(mx, my)) return i;
    }
    return -1;
}

// 根据当前轨迹模式实时更新曲线数据
//   折线段模式（0）已可用 — 清空曲线数据，直接渲染关键帧连线
//   Hermite模式（1）需学生完成 generateCurveThroughPoints() 后生效
static void regenerateCurveForMode(AppState* state, HWND hWnd) {
    if (!state || state->keyframes.size() < 2) {
        state->curvePoints.clear();
        return;
    }
    if (state->trajectoryMode == 1) {
        // TODO 学生: 完成 generateCurveThroughPoints() 后此分支自动生效
        RECT rc;
        GetClientRect(hWnd, &rc);
        float sw = (float)(rc.right - rc.left);
        float sh = (float)(rc.bottom - rc.top);
        state->curvePoints = generateCurveThroughPoints(state->keyframes, sw, sh);
    } else {
        state->curvePoints.clear();  // 折线段模式直接渲染关键帧连线
    }
}

// ==================== OpenGL窗口过程 ====================
static LRESULT CALLBACK GLViewWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // 从父窗口获取AppState
    HWND hParent = GetParent(hWnd);
    AppState* state = (AppState*)GetWindowLongPtr(hParent, GWLP_USERDATA);

    switch (msg) {
    case WM_CREATE: {
        // 设置OpenGL像素格式
        PIXELFORMATDESCRIPTOR pfd = {
            sizeof(PIXELFORMATDESCRIPTOR), 1,
            PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
            PFD_TYPE_RGBA, 32,
            0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0,
            24, 8, 0,
            PFD_MAIN_PLANE, 0, 0, 0, 0
        };

        HDC hdc = GetDC(hWnd);
        int pixelFormat = ChoosePixelFormat(hdc, &pfd);
        SetPixelFormat(hdc, pixelFormat, &pfd);
        HGLRC hrc = wglCreateContext(hdc);
        wglMakeCurrent(hdc, hrc);

        // 创建字体显示列表（用于时间轴刻度文字）
        HFONT hFont = CreateFontW(
            12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
            ANTIALIASED_QUALITY, FF_DONTCARE | DEFAULT_PITCH, L"Consolas");
        if (hFont) {
            SelectObject(hdc, hFont);
            g_fontBase = glGenLists(128);
            wglUseFontBitmapsW(hdc, 0, 128, g_fontBase);
            DeleteObject(hFont);
        }
        ReleaseDC(hWnd, hdc);

        // 存储HGLRC
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)hrc);
        return 0;
    }

    case WM_DESTROY: {
        HGLRC hrc = (HGLRC)GetWindowLongPtr(hWnd, GWLP_USERDATA);
        if (hrc) {
            wglMakeCurrent(nullptr, nullptr);
            wglDeleteContext(hrc);
        }
        if (g_fontBase) {
            glDeleteLists(g_fontBase, 128);
            g_fontBase = 0;
        }
        KillTimer(hWnd, TIMER_ANIM);
        return 0;
    }

    case WM_SIZE: {
        // OpenGL视口已在renderScene中通过窗口大小设置
        if (state) {
            InvalidateRect(hWnd, nullptr, FALSE);
        }
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        if (state) {
            RECT rc;
            GetClientRect(hWnd, &rc);
            int w = rc.right - rc.left;
            int h = rc.bottom - rc.top;
            renderScene(state, w, h);
            SwapBuffers(hdc);
        }
        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_TIMER: {
        if (wParam == TIMER_ANIM && state && state->isPlaying) {
            if (state->keyframes.size() > 1) {
                state->animationTime += 0.005f * state->animationSpeed;
                if (state->animationTime > 1.0f)
                    state->animationTime = 0.0f;

                // 同步滑块位置
                state->sliderPos = (int)(state->animationTime * 1000.0f);

                // 更新UI面板（守卫函数会检查是否真正需要重绘）
                InvalidateUIPanelIfChanged(state);

                InvalidateRect(hWnd, nullptr, FALSE);
            }
        }
        return 0;
    }

    // ==================== 鼠标事件 ====================
    case WM_LBUTTONDOWN: {
        if (!state) return 0;
        Point2f pt = screenToGLCoords(lParam);

        // 【流程1】切向量编辑中，点击=确认
        if (state->editStep == EditStep::EditingTangent && state->editingIndex >= 0) {
            state->editStep = EditStep::Idle;
            state->selectedIndex = state->editingIndex;
            if (state->selectedIndex >= 0 && state->selectedIndex < (int)state->keyframes.size()) {
                state->keyframes[state->selectedIndex].selected = true;
            }
            swprintf_s(state->statusText,
                L"✅ 关键帧%d切向量调整完成，可继续编辑其他关键帧", state->editingIndex);
            state->statusType = 0;
            state->editingIndex = -1;
            InvalidateRect(hWnd, nullptr, FALSE);
            InvalidateUIPanelIfChanged(state);
            return 0;
        }

        // 【流程2】检测是否点击已有关键帧（逆向遍历，25px命中区）
        for (int i = (int)state->keyframes.size() - 1; i >= 0; i--) {
            if (state->keyframes[i].ContainsPoint(pt.x, pt.y)) {
                // 进入位置编辑模式
                state->editStep = EditStep::EditingPos;
                state->hasPendingKeyframe = false;
                for (int j = 0; j < (int)state->keyframes.size(); j++)
                    state->keyframes[j].selected = (j == i);
                state->selectedIndex = i;
                state->editingIndex = i;
                swprintf_s(state->statusText,
                    L"📍 选中关键帧%d，拖拽移动位置，松开后调整切向量", i);
                state->statusType = 1;
                InvalidateRect(hWnd, nullptr, FALSE);
                InvalidateUIPanelIfChanged(state);
                return 0;
            }
        }

        // 【流程3】点击空白处 → 新增关键帧
        for (auto& kf : state->keyframes) kf.selected = false;
        state->selectedIndex = -1;
        state->editStep = EditStep::Adding;
        state->pendingKeyframe = Keyframe(pt.x, pt.y, (float)state->keyframes.size());
        state->pendingKeyframe.selected = true;
        state->hasPendingKeyframe = true;
        wcscpy_s(state->statusText, L"➕ 新增关键帧：拖拽调整箭头方向，松开确认");
        state->statusType = 3;
        InvalidateRect(hWnd, nullptr, FALSE);
        InvalidateUIPanelIfChanged(state);
        return 0;
    }

    case WM_MOUSEMOVE: {
        if (!state) return 0;
        Point2f pt = screenToGLCoords(lParam);

        // 【流程1】新增关键帧：拖拽调整箭头（方向+幅度=导数dP/dt）
        if (state->editStep == EditStep::Adding && state->hasPendingKeyframe) {
            float dx = pt.x - state->pendingKeyframe.x;
            float dy = pt.y - state->pendingKeyframe.y;
            float len = sqrtf(dx * dx + dy * dy);
            if (len > 5.0f) {
                state->pendingKeyframe.tangentX = dx;
                state->pendingKeyframe.tangentY = dy;
            }
            wcscpy_s(state->statusText, L"➕ 新增关键帧：调整箭头方向，松开确认");
            state->statusType = 3;
            InvalidateRect(hWnd, nullptr, FALSE);
            InvalidateUIPanelIfChanged(state);
            return 0;
        }

        // 【流程2】编辑位置：拖拽移动关键帧
        if (state->editStep == EditStep::EditingPos && state->editingIndex >= 0
            && state->editingIndex < (int)state->keyframes.size()) {
            Keyframe& kf = state->keyframes[state->editingIndex];
            kf.x = pt.x;
            kf.y = pt.y;
            kf.tangentUserEdited = false;  // 位置改变后旧切线已失效，标记需重新计算
            regenerateCurveForMode(state, hWnd);
            swprintf_s(state->statusText,
                L"📍 拖拽移动关键帧，松开后调整切向量");
            state->statusType = 1;
            InvalidateRect(hWnd, nullptr, FALSE);
            InvalidateUIPanelIfChanged(state);
            return 0;
        }

        // 【流程3】编辑切向量：移动鼠标调箭头（方向+幅度=导数dP/dt）
        if (state->editStep == EditStep::EditingTangent && state->editingIndex >= 0
            && state->editingIndex < (int)state->keyframes.size()) {
            Keyframe& kf = state->keyframes[state->editingIndex];
            float dx = pt.x - kf.x, dy = pt.y - kf.y;
            float len = sqrtf(dx * dx + dy * dy);
            if (len > 5.0f) {
                kf.tangentX = dx;
                kf.tangentY = dy;
                kf.tangentUserEdited = true;  // 用户显式编辑，重新生成曲线时保留
            }
            regenerateCurveForMode(state, hWnd);
            swprintf_s(state->statusText, L"✏️ 调整关键帧%d切向量，点击确认完成", state->editingIndex);
            state->statusType = 2;
            InvalidateRect(hWnd, nullptr, FALSE);
            InvalidateUIPanelIfChanged(state);
            return 0;
        }

        // 【空闲时】检测悬停关键帧，切换鼠标光标为手型
        if (state->editStep == EditStep::Idle) {
            int hitIdx = hitTestKeyframes(state->keyframes, pt.x, pt.y);
            if (hitIdx >= 0) {
                SetCursor(LoadCursor(nullptr, IDC_HAND));
            } else {
                SetCursor(LoadCursor(nullptr, IDC_CROSS));
            }
        }

        // 追踪鼠标离开（用于取消新增操作）
        {
            TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, hWnd, 0 };
            TrackMouseEvent(&tme);
        }
        return 0;
    }

    case WM_SETCURSOR: {
        // 在客户端区阻止 Windows 默认光标重置
        if (LOWORD(lParam) == HTCLIENT) {
            return TRUE;
        }
        break;
    }

    case WM_LBUTTONUP: {
        if (!state) return 0;

        // 【流程1】新增关键帧：松开=添加并立即进入切向量编辑模式
        if (state->editStep == EditStep::Adding && state->hasPendingKeyframe) {
            float newX = state->pendingKeyframe.x;
            float newY = state->pendingKeyframe.y;
            // 标记用户已编辑切向量（在Adding拖拽阶段设置的），防止Catmull-Rom覆盖
            float tLen = sqrtf(state->pendingKeyframe.tangentX * state->pendingKeyframe.tangentX
                             + state->pendingKeyframe.tangentY * state->pendingKeyframe.tangentY);
            if (tLen > 1.5f) state->pendingKeyframe.tangentUserEdited = true;
            state->keyframes.push_back(state->pendingKeyframe);
            state->hasPendingKeyframe = false;
            sortKeyframes(state->keyframes);
            regenerateCurveForMode(state, hWnd);

            // 排序后找到新增关键帧的新索引
            int newIdx = -1;
            for (int ki = 0; ki < (int)state->keyframes.size(); ki++) {
                if (fabsf(state->keyframes[ki].x - newX) < 0.5f &&
                    fabsf(state->keyframes[ki].y - newY) < 0.5f) {
                    newIdx = ki; break;
                }
            }
            if (newIdx >= 0) {
                for (auto& kf : state->keyframes) kf.selected = false;
                state->keyframes[newIdx].selected = true;
                state->selectedIndex = newIdx;
                state->editingIndex = newIdx;
                state->editStep = EditStep::EditingTangent;
                swprintf_s(state->statusText, L"✏️ 关键帧已添加，移动鼠标调整切向量，点击确认完成");
                state->statusType = 2;
            } else {
                state->editStep = EditStep::Idle;
                swprintf_s(state->statusText, L"✅ 关键帧已添加");
                state->statusType = 0;
            }
            InvalidateRect(hWnd, nullptr, FALSE);
            InvalidateUIPanelIfChanged(state);
            return 0;
        }

        // 【流程2】编辑位置：松开=进入切向量编辑模式
        if (state->editStep == EditStep::EditingPos && state->editingIndex >= 0) {
            state->editStep = EditStep::EditingTangent;
            swprintf_s(state->statusText,
                L"✏️ 移动鼠标调整关键帧%d切向量，点击确认完成", state->editingIndex);
            state->statusType = 2;
            InvalidateRect(hWnd, nullptr, FALSE);
            InvalidateUIPanelIfChanged(state);
            return 0;
        }
        return 0;
    }

    case WM_MOUSELEAVE: {
        if (state && state->editStep == EditStep::Adding) {
            wcscpy_s(state->statusText, L"已取消新增");
            state->statusType = 0;
            state->hasPendingKeyframe = false;
            state->editStep = EditStep::Idle;
            InvalidateRect(hWnd, nullptr, FALSE);
            InvalidateUIPanelIfChanged(state);
        }
        return 0;
    }

    // ==================== 自定义消息（来自UI面板） ====================
    case WM_PLAYBACK_TOGGLE: {
        if (state) {
            state->isPlaying = !state->isPlaying;
            if (state->isPlaying) {
                SetTimer(hWnd, TIMER_ANIM, ANIM_TIMER_INTERVAL, nullptr);
                wcscpy_s(state->statusText, L"▶️ 动画播放中...");
                state->statusType = 0;
            } else {
                KillTimer(hWnd, TIMER_ANIM);
                wcscpy_s(state->statusText, L"⏸️ 动画已暂停");
                state->statusType = 0;
            }
            InvalidateUIPanelIfChanged(state);
        }
        return 0;
    }

    case WM_RESET_ANIM: {
        if (state) {
            state->isPlaying = false;
            KillTimer(hWnd, TIMER_ANIM);
            state->animationTime = 0.0f;
            state->sliderPos = 0;
            wcscpy_s(state->statusText, L"🔄 动画已重置");
            state->statusType = 0;
            InvalidateRect(hWnd, nullptr, FALSE);
            InvalidateUIPanelIfChanged(state);
        }
        return 0;
    }

    case WM_DELETE_SELECTED: {
        if (state) {
            state->DeleteSelected();
            regenerateCurveForMode(state, hWnd);
            InvalidateRect(hWnd, nullptr, FALSE);
            InvalidateUIPanelIfChanged(state);
        }
        return 0;
    }

    case WM_CLEAR_ALL: {
        if (state) {
            KillTimer(hWnd, TIMER_ANIM);
            state->ClearAll();
            InvalidateRect(hWnd, nullptr, FALSE);
            InvalidateUIPanelIfChanged(state);
        }
        return 0;
    }

    case WM_SET_ANIM_TIME: {
        if (state) {
            state->isPlaying = false;
            KillTimer(hWnd, TIMER_ANIM);
            state->animationTime = (float)wParam / 1000.0f;
            state->sliderPos = (int)wParam;
            InvalidateRect(hWnd, nullptr, FALSE);
            InvalidateUIPanelIfChanged(state);
        }
        return 0;
    }

    case WM_SET_ANIM_SPEED: {
        if (state) {
            state->speedSliderPos = (int)wParam;
            state->animationSpeed = (float)wParam / 100.0f;
            InvalidateUIPanelIfChanged(state);
        }
        return 0;
    }

    case WM_SET_TRAJECTORY: {
        if (state) {
            state->trajectoryMode = (int)wParam;  // 0=折线段, 1=Hermite
            // 切换到Hermite模式时自动生成曲线
            regenerateCurveForMode(state, hWnd);
            InvalidateRect(hWnd, nullptr, FALSE);
            InvalidateUIPanelIfChanged(state);
        }
        return 0;
    }

    // 键盘快捷键
    case WM_KEYDOWN: {
        if (!state) return 0;
        if ((wParam == VK_DELETE || wParam == VK_BACK) && state->selectedIndex >= 0) {
            state->DeleteSelected();
            regenerateCurveForMode(state, hWnd);
            InvalidateRect(hWnd, nullptr, FALSE);
            InvalidateUIPanelIfChanged(state);
        }
        if (wParam == VK_ESCAPE) {
            state->editStep = EditStep::Idle;
            state->hasPendingKeyframe = false;
            state->editingIndex = -1;
            state->selectedIndex = -1;
            for (auto& kf : state->keyframes) kf.selected = false;
            wcscpy_s(state->statusText, L"✅ 操作已取消");
            state->statusType = 0;
            InvalidateRect(hWnd, nullptr, FALSE);
            InvalidateUIPanelIfChanged(state);
        }
        return 0;
    }

    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
}

// ==================== 注册与创建 ====================

void RegisterGLViewClass(HINSTANCE hInstance) {
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = GLViewWndProc;
    wc.hInstance = hInstance;
    wc.hCursor = nullptr;  // 由 WM_MOUSEMOVE 动态设置光标
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = L"OpenGLCanvas";
    RegisterClassEx(&wc);
}

HWND CreateGLView(HINSTANCE hInstance, HWND hParent, int x, int y, int w, int h, AppState* state) {
    HWND hWnd = CreateWindowEx(
        0, L"OpenGLCanvas", nullptr,
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        x, y, w, h,
        hParent, nullptr, hInstance, nullptr
    );

    if (hWnd) {
        state->hGLView = hWnd;
        SetFocus(hWnd);  // 确保键盘输入到OpenGL窗口
    }
    return hWnd;
}
