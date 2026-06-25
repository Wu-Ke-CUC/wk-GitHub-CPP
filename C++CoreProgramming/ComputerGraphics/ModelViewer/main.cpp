// ============================================================
// ModelViewer - Win32 OpenGL + GDI+ Interactive Panel
// Converted from GLUT to Windows native application
// Support Real-time Skeletal Anim (FBX) & Static Models (OBJ)
// Visual Studio 2019 / Win32 Window Application
// ============================================================

#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include <gdiplus.h>
#include <commctrl.h>
#include <commdlg.h>
#include "Model.h"
#include "Camera.h"
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <cassert>
#include <cstdio>
#include <cmath>

using namespace Gdiplus;
using namespace std;

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")

// ============================================================
// 界面UI暗色配色方案定义
// ============================================================
#define CLR_BG_MAIN          RGB(30, 30, 30)
#define CLR_BG_PANEL         RGB(37, 37, 38)
#define CLR_BG_HEADER        RGB(45, 45, 48)
#define CLR_BG_BUTTON        RGB(60, 60, 65)
#define CLR_BG_BUTTON_HOVER  RGB(80, 80, 88)
#define CLR_BG_BUTTON_ACTIVE RGB(0, 122, 204)
#define CLR_BG_KEY           RGB(50, 50, 55)
#define CLR_TEXT              RGB(220, 220, 220)
#define CLR_TEXT_DIM          RGB(160, 160, 165)
#define CLR_TEXT_HEADER       RGB(86, 156, 214)
#define CLR_TEXT_ACCENT       RGB(78, 201, 176)
#define CLR_BORDER            RGB(70, 70, 75)
#define CLR_SEPARATOR         RGB(55, 55, 60)
#define CLR_TOGGLE_ON         RGB(0, 180, 120)
#define CLR_TOGGLE_OFF        RGB(180, 60, 60)

#define PANEL_WIDTH           340
#define PANEL_PADDING         12
#define BTN_HEIGHT            32
#define SECTION_GAP           6
#define PANEL_LEFT_PAD        10

// ????????
enum ViewMode { VIEW_WIREFRAME = 0, VIEW_SOLID = 1, VIEW_SKELETON = 2 };


// ============================================================
// 前向函数声明清单
// ============================================================
LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK OpenGLWndProc(HWND, UINT, WPARAM, LPARAM);
void InitOpenGL(HWND hWnd);
void ResizeOpenGL(int w, int h);
void DrawOpenGLScene();

void ActionCycleViewMode();
void ActionToggleProjection();
void ActionResetAll();
void ActionQuit();
void ActionOpenModel();

// UI
enum BtnType { BTN_TOGGLE, BTN_DIRECTION, BTN_ACTION, BTN_HEADER, BTN_INFO, BTN_DROPDOWN };
struct GButton {
    RECT rect;
    BtnType type;
    const wchar_t* label;
    const wchar_t* keyText;
    const wchar_t* keyText2;
    int actionId;
    bool hover;
    bool pressed;
    int hoverSub;
};

enum ActionId {
    AID_TOGGLE_WIREFRAME = 1,
    AID_TOGGLE_PROJECTION,
    AID_RESET_ALL,
    AID_QUIT,
    AID_OPEN_MODEL,
    AID_TOGGLE_ANIMATION,
    AID_VIEW_MODE
};

// ============================================================
// 前向函数声明清单
// ============================================================
static HWND g_hMainWnd = NULL;
static HWND g_hGLWnd = NULL;
static HDC g_hGLDC = NULL;
static HGLRC g_hGLRC = NULL;
static int g_glWidth = 0, g_glHeight = 0;

static Model    g_model;
static Camera   g_camera;
static GLuint   g_displayList = 0;

static int g_viewMode = VIEW_WIREFRAME;  // 0=?? 1=?? 2=??
static int g_isOrtho = 1;
static bool g_modelLoaded = false;
static bool g_animPlaying = false;

static bool  g_mouseLDown = false;
static bool  g_mouseRDown = false;
static int   g_lastMouseX = 0;
static int   g_lastMouseY = 0;

static GButton g_buttons[32];
static int g_btnCount = 0;
static int g_panelScrollOffset = 0;
static int g_panelContentHeight = 0;
static int g_hoverBtn = -1;

// 性能监控动态标签缓冲区
static wchar_t g_fpsLabel[64] = L"实时帧率: -- FPS";
static wchar_t g_boneCountLabel[64] = L"骨骼数量: --";
static float g_currentFPS = 0.0f;

// 检视模式
static bool g_viewModeDropdownOpen = false;
static int g_viewModeDropdownHover = -1;
static const wchar_t* g_viewModeOptions[] = { L"线框网格", L"实体渲染", L"骨骼显示" };
static const int g_viewModeOptionCount = 3;


static GdiplusStartupInput g_gdiStartupInput;
static ULONG_PTR g_gdiToken = 0;

// UI 按钮类型枚举
void AddButton(BtnType type, const wchar_t* label, const wchar_t* key, const wchar_t* key2, int actionId, int y, int h = BTN_HEIGHT) {
    GButton& b = g_buttons[g_btnCount];
    b.type = type;
    b.label = label;
    b.keyText = key;
    b.keyText2 = key2;
    b.actionId = actionId;
    b.hover = false;
    b.pressed = false;
    b.hoverSub = 0;
    SetRect(&b.rect, PANEL_LEFT_PAD, y, PANEL_WIDTH - PANEL_PADDING, y + h);
    g_btnCount++;
}

// 初始化右侧功能面板按钮布局
void InitPanelButtons() {
    g_btnCount = 0;
    int y = PANEL_PADDING + 8;

    AddButton(BTN_HEADER, L"3D模型检视器(FBX)", L"", L"", 0, y, 36);
    y += 42;

    AddButton(BTN_HEADER, L"► 文件操作", L"", L"", 0, y, 26);
    y += 30;
    AddButton(BTN_ACTION, L"加载外部模型文件...", L"FBX", L"", AID_OPEN_MODEL, y);
    y += BTN_HEIGHT + SECTION_GAP + 4;

    AddButton(BTN_HEADER, L"► 视图模式", L"", L"", 0, y, 26);
    y += 30;
    AddButton(BTN_DROPDOWN, L"检视模式", L"Space", L"", AID_VIEW_MODE, y);
    y += BTN_HEIGHT + 4;
    AddButton(BTN_TOGGLE, L" 正交 / 透视", L"V", L"", AID_TOGGLE_PROJECTION, y);
    y += BTN_HEIGHT + SECTION_GAP + 4;


    AddButton(BTN_HEADER, L"► 动画控制", L"", L"", 0, y, 26);
    y += 30;
    AddButton(BTN_TOGGLE, L"动画播放 / 暂停", L"P", L"", AID_TOGGLE_ANIMATION, y);
    y += BTN_HEIGHT + SECTION_GAP + 4;
    AddButton(BTN_HEADER, L"► 系统全局管理", L"", L"", 0, y, 26);
    y += 30;
    AddButton(BTN_ACTION, L"重置默认视角", L"S", L"", AID_RESET_ALL, y);
    y += BTN_HEIGHT + 4;
    AddButton(BTN_ACTION, L"退出系统", L"Q", L"", AID_QUIT, y);
    y += BTN_HEIGHT + SECTION_GAP + 4;

    AddButton(BTN_HEADER, L"► 鼠标交互说明", L"", L"", 0, y, 26);
    y += 30;
    AddButton(BTN_INFO, L"按住左键拖动   → 旋转模型", L"", L"", 0, y, 28);
    y += 28 + 3;
    AddButton(BTN_INFO, L"按住右键拖动   → 平移镜头", L"", L"", 0, y, 28);
    y += 28 + 3;
    AddButton(BTN_INFO, L"滚轮滚动   → 缩放模型", L"", L"", 0, y, 28);
    y += 28 + PANEL_PADDING;

    AddButton(BTN_HEADER, L"► 性能监控", L"", L"", 0, y, 26);
    y += 30;
    AddButton(BTN_INFO, g_fpsLabel, L"", L"", 0, y, 28);
    y += 28 + 3;
    AddButton(BTN_INFO, g_boneCountLabel, L"", L"", 0, y, 28);
    y += 28 + PANEL_PADDING;

    g_panelContentHeight = y;
}

// 胶囊体绘制辅助函数：在两点之间绘制带半球端盖的圆柱体
static void DrawCapsule(float x1, float y1, float z1, float x2, float y2, float z2,
                        float radius, GLUquadric* quad, int slices, int stacks) {
    float dx = x2 - x1, dy = y2 - y1, dz = z2 - z1;
    float len = sqrtf(dx * dx + dy * dy + dz * dz);
    if (len < 0.0001f) {
        glPushMatrix();
        glTranslatef(x1, y1, z1);
        gluSphere(quad, radius, slices, stacks);
        glPopMatrix();
        return;
    }
    dx /= len; dy /= len; dz /= len;
    float dot = dz;
    float angle = acosf(dot) * 180.0f / 3.14159265f;
    float ax = -dy, ay = dx, az = 0.0f;
    float axLen = sqrtf(ax * ax + ay * ay);
    float shaftLen = len - 2.0f * radius;
    if (shaftLen < 0.0f) shaftLen = 0.0f;
    glPushMatrix();
    glTranslatef(x1, y1, z1);
    if (axLen > 0.0001f) {
        glRotatef(angle, ax / axLen, ay, az / axLen);
    }
    else if (dot < 0.0f) {
        glRotatef(180.0f, 1.0f, 0.0f, 0.0f);
    }
    gluSphere(quad, radius, slices, stacks);
    if (shaftLen > 0.0f) {
        glTranslatef(0.0f, 0.0f, radius);
        gluCylinder(quad, radius, radius, shaftLen, slices, stacks);
        glTranslatef(0.0f, 0.0f, shaftLen);
    }
    gluSphere(quad, radius, slices, stacks);
    glPopMatrix();
}

// 骨骼系统检视渲染：胶囊体表示骨骼，圆球表示关节
static void DrawSkeleton() {
    if (!g_modelLoaded || !g_model.is_fbx || g_model.bones.empty()) return;
    static GLUquadric* s_skelQuad = nullptr;
    if (!s_skelQuad) s_skelQuad = gluNewQuadric();
    gluQuadricDrawStyle(s_skelQuad, GLU_FILL);
    gluQuadricNormals(s_skelQuad, GLU_SMOOTH);
    const float jointRadius = 0.025f;
    const float boneRadius = 0.012f;
    const int slices = 12;
    const int stacks = 6;
    float sFactor = (g_model.max_scale > 0.0001f) ? (1.5f / g_model.max_scale) : 1.0f;
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    for (size_t b = 0; b < g_model.bones.size(); b++) {
        int parentIdx = g_model.bones[b].parentIndex;
        if (parentIdx >= 0 && parentIdx < (int)g_model.bones.size()) {
            float p1x = ((float)g_model.bones[parentIdx].currentGlobalPos[0] - g_model.mean_x) * sFactor;
            float p1y = ((float)g_model.bones[parentIdx].currentGlobalPos[1] - g_model.mean_y) * sFactor;
            float p1z = ((float)g_model.bones[parentIdx].currentGlobalPos[2] - g_model.mean_z) * sFactor;
            float p2x = ((float)g_model.bones[b].currentGlobalPos[0] - g_model.mean_x) * sFactor;
            float p2y = ((float)g_model.bones[b].currentGlobalPos[1] - g_model.mean_y) * sFactor;
            float p2z = ((float)g_model.bones[b].currentGlobalPos[2] - g_model.mean_z) * sFactor;
            glColor3f(0.3f, 0.7f, 0.9f);
            DrawCapsule(p1x, p1y, p1z, p2x, p2y, p2z, boneRadius, s_skelQuad, slices, stacks);
        }
    }
    for (size_t b = 0; b < g_model.bones.size(); b++) {
        float px = ((float)g_model.bones[b].currentGlobalPos[0] - g_model.mean_x) * sFactor;
        float py = ((float)g_model.bones[b].currentGlobalPos[1] - g_model.mean_y) * sFactor;
        float pz = ((float)g_model.bones[b].currentGlobalPos[2] - g_model.mean_z) * sFactor;
        glPushMatrix();
        glTranslatef(px, py, pz);
        if (g_model.bones[b].parentIndex == -1)
            glColor3f(1.0f, 0.35f, 0.35f);
        else
            glColor3f(0.3f, 0.85f, 1.0f);
        gluSphere(s_skelQuad, jointRadius, slices, stacks);
        glPopMatrix();
    }
    glEnable(GL_LIGHTING);
}

// 绘制圆角矩形 UI 边框辅助方法
void DrawRoundRect(Graphics& g, Pen* pen, Brush* brush, int x, int y, int w, int h, int r) {
    GraphicsPath path;
    path.AddArc(x, y, r * 2, r * 2, 180, 90);
    path.AddArc(x + w - r * 2, y, r * 2, r * 2, 270, 90);
    path.AddArc(x + w - r * 2, y + h - r * 2, r * 2, r * 2, 0, 90);
    path.AddArc(x, y + h - r * 2, r * 2, r * 2, 90, 90);
    path.CloseFigure();
    if (brush) g.FillPath(brush, &path);
    if (pen) g.DrawPath(pen, &path);
}

// 将 COLORREF 转换为 GDI+ Color
Color ToGdiColor(COLORREF c, BYTE a = 255) {
    return Color(a, GetRValue(c), GetGValue(c), GetBValue(c));
}

// 双缓冲刷新渲染右侧高级 GDI+ 面板
void DrawPanel(HDC hdc, RECT& panelRect) {
    int pw = panelRect.right - panelRect.left;
    int ph = panelRect.bottom - panelRect.top;

    Bitmap backBuffer(pw, ph);
    Graphics g(&backBuffer);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintAntiAlias);

    SolidBrush bgBrush(ToGdiColor(CLR_BG_PANEL));
    g.FillRectangle(&bgBrush, 0, 0, pw, ph);

    Pen borderPen(ToGdiColor(CLR_BORDER), 1.0f);
    g.DrawLine(&borderPen, 0, 0, 0, ph);

    SolidBrush titleBg(ToGdiColor(CLR_BG_HEADER));
    g.FillRectangle(&titleBg, 0, 0, pw, 50);

    Pen sepPen(ToGdiColor(CLR_SEPARATOR), 1.0f);
    g.DrawLine(&sepPen, 0, 50, pw, 50);

    Font sectionFont(L"Segoe UI", 9, FontStyleBold);
    Font btnFont(L"Segoe UI", 9);
    Font keySmallFont(L"Segoe UI", 8, FontStyleBold);

    SolidBrush sectionBrush(ToGdiColor(CLR_TEXT_DIM));
    SolidBrush btnTextBrush(ToGdiColor(CLR_TEXT));
    SolidBrush keyBrush(ToGdiColor(CLR_TEXT_HEADER));
    SolidBrush toggleOnBrush(ToGdiColor(CLR_TOGGLE_ON));
    SolidBrush toggleOffBrush(ToGdiColor(CLR_TOGGLE_OFF));
    Pen btnBorder(ToGdiColor(CLR_BORDER), 1.0f);

    for (int i = 0; i < g_btnCount; i++) {
        GButton& b = g_buttons[i];
        int bx = b.rect.left;
        int by = b.rect.top - g_panelScrollOffset;
        int bw = b.rect.right - b.rect.left;
        int bh = b.rect.bottom - b.rect.top;

        if (by + bh < 50 || by > ph) continue;

        if (b.type == BTN_HEADER) {
            g.DrawString(b.label, -1, &sectionFont, PointF((float)bx + 2, (float)by + 4), &sectionBrush);
            continue;
        }

        Color btnBgClr = b.hover ? ToGdiColor(CLR_BG_BUTTON_HOVER) : ToGdiColor(CLR_BG_BUTTON);
        if (b.pressed) btnBgClr = ToGdiColor(CLR_BG_BUTTON_ACTIVE);
        SolidBrush btnBg(btnBgClr);
        DrawRoundRect(g, &btnBorder, &btnBg, bx, by, bw, bh, 5);

        int textX = bx + 10;
        int textY = by + (bh - 16) / 2;

        if (b.type == BTN_TOGGLE) {
            g.DrawString(b.label, -1, &btnFont, PointF((float)textX, (float)textY), &btnTextBrush);

            bool state = false;
            if (b.actionId == AID_TOGGLE_WIREFRAME) state = false;
            else if (b.actionId == AID_TOGGLE_PROJECTION) state = !g_isOrtho;
            else if (b.actionId == AID_TOGGLE_ANIMATION) state = g_animPlaying;

            int indX = bx + bw - 55;
            int indY = by + (bh - 14) / 2;
            SolidBrush* indBrush = state ? &toggleOnBrush : &toggleOffBrush;
            DrawRoundRect(g, NULL, indBrush, indX, indY, 44, 14, 7);
            Font indFont(L"Segoe UI", 7, FontStyleBold);
            SolidBrush indText(Color(255, 255, 255, 255));
            g.DrawString(state ? L"ON" : L"OFF", -1, &indFont, PointF((float)indX + (state ? 14 : 10), (float)indY + 1), &indText);

            int kx = bx + bw - 92;
            int ky = by + (bh - 18) / 2;
            SolidBrush keyBg(ToGdiColor(CLR_BG_KEY));
            DrawRoundRect(g, NULL, &keyBg, kx, ky, 32, 18, 4);
            g.DrawString(b.keyText, -1, &keySmallFont, PointF((float)kx + 4, (float)ky + 3), &keyBrush);

        }
        else if (b.type == BTN_ACTION) {
            g.DrawString(b.label, -1, &btnFont, PointF((float)textX, (float)textY), &btnTextBrush);

            int kx = bx + bw - 46;
            int ky = by + (bh - 18) / 2;
            SolidBrush keyBg(ToGdiColor(CLR_BG_KEY));
            DrawRoundRect(g, NULL, &keyBg, kx, ky, 32, 18, 4);
            g.DrawString(b.keyText, -1, &keySmallFont, PointF((float)kx + 8, (float)ky + 3), &keyBrush);
        }
        else if (b.type == BTN_INFO) {
            g.DrawString(b.label, -1, &btnFont, PointF((float)textX, (float)textY), &sectionBrush);
        }
        else if (b.type == BTN_DROPDOWN) {
            Color btnBgClr2 = b.hover ? ToGdiColor(CLR_BG_BUTTON_HOVER) : ToGdiColor(CLR_BG_BUTTON);
            if (b.pressed) btnBgClr2 = ToGdiColor(CLR_BG_BUTTON_ACTIVE);
            SolidBrush btnBg2(btnBgClr2);
            DrawRoundRect(g, &btnBorder, &btnBg2, bx, by, bw, bh, 5);
            const wchar_t* modeText = g_viewModeOptions[g_viewMode];
            g.DrawString(modeText, -1, &btnFont, PointF((float)textX, (float)textY), &btnTextBrush);
            Font arrowFont(L"Segoe UI", 8);
            g.DrawString(g_viewModeDropdownOpen ? L"" : L"", -1, &arrowFont,
                PointF((float)(bx + bw - 30), (float)(by + (bh - 16) / 2)), &btnTextBrush);
            int kx = bx + bw - 62;
            int ky = by + (bh - 18) / 2;
            SolidBrush keyBg(ToGdiColor(CLR_BG_KEY));
            DrawRoundRect(g, NULL, &keyBg, kx, ky, 32, 18, 4);
            g.DrawString(b.keyText, -1, &keySmallFont, PointF((float)kx + 4, (float)ky + 3), &keyBrush);
        }
    }

    if (g_viewModeDropdownOpen) {
        for (int i = 0; i < g_btnCount; i++) {
            if (g_buttons[i].type == BTN_DROPDOWN && g_buttons[i].actionId == AID_VIEW_MODE) {
                GButton& db = g_buttons[i];
                int dbx = db.rect.left;
                int dby = db.rect.top + (db.rect.bottom - db.rect.top) - g_panelScrollOffset;
                int dbw = db.rect.right - db.rect.left;
                for (int oi = 0; oi < g_viewModeOptionCount; oi++) {
                    int oy = dby + oi * 28;
                    if (oy > ph) break;
                    Color optBgClr = (oi == g_viewModeDropdownHover) ? ToGdiColor(CLR_BG_BUTTON_HOVER) : ToGdiColor(CLR_BG_KEY);
                    SolidBrush optBg(optBgClr);
                    Pen optBorder(ToGdiColor(CLR_BORDER), 1.0f);
                    DrawRoundRect(g, &optBorder, &optBg, dbx + 8, oy, dbw - 16, 26, 4);
                    SolidBrush* optTextBr = (oi == g_viewMode) ? &toggleOnBrush : &btnTextBrush;
                    g.DrawString(g_viewModeOptions[oi], -1, &btnFont, PointF((float)dbx + 18, (float)oy + 5), optTextBr);
                }
                break;
            }
        }
    }

    Graphics screen(hdc);
    screen.DrawImage(&backBuffer, (INT)panelRect.left, (INT)panelRect.top, 0, 0, pw, ph, UnitPixel);
}

int HitTestButtons(int mx, int my) {
    for (int i = 0; i < g_btnCount; i++) {
        GButton& b = g_buttons[i];
        if (b.type == BTN_HEADER || b.type == BTN_INFO) continue;
        if (b.type == BTN_DROPDOWN) {
            RECT r = b.rect;
            r.top -= g_panelScrollOffset;
            r.bottom -= g_panelScrollOffset;
            if (mx >= r.left && mx <= r.right && my >= r.top && my <= r.bottom) return i;
            continue;
        }
        RECT r = b.rect;
        r.top -= g_panelScrollOffset;
        r.bottom -= g_panelScrollOffset;
        if (mx >= r.left && mx <= r.right && my >= r.top && my <= r.bottom) return i;
    }
    return -1;
}

void ExecuteAction(int actionId) {
    switch (actionId) {
    case AID_VIEW_MODE: {
        g_viewModeDropdownOpen = !g_viewModeDropdownOpen;
        InvalidateRect(g_hMainWnd, NULL, FALSE);
        break;
    }
    case AID_TOGGLE_PROJECTION: ActionToggleProjection(); break;
    case AID_RESET_ALL:         ActionResetAll(); break;
    case AID_QUIT:              ActionQuit(); break;
    case AID_OPEN_MODEL:        ActionOpenModel(); break;
    case AID_TOGGLE_ANIMATION:  g_animPlaying = !g_animPlaying; InvalidateRect(g_hMainWnd, NULL, FALSE); break;
    }
}

void ActionCycleViewMode() {
    g_viewMode = (g_viewMode + 1) % 3;
    InvalidateRect(g_hGLWnd, NULL, FALSE);
    InvalidateRect(g_hMainWnd, NULL, FALSE);
}

void ActionToggleProjection() {
    g_isOrtho = !g_isOrtho;
    HDC hdc = GetDC(g_hGLWnd);
    wglMakeCurrent(hdc, g_hGLRC);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspect = (g_glHeight > 0) ? (float)g_glWidth / (float)g_glHeight : 1.0f;
    if (g_isOrtho) {
        float s = g_camera.camera_distance * 0.35f;
        glOrtho(-s * aspect, s * aspect, -s, s, 0.01, 200.0);
    }
    else {
        gluPerspective(45.0, aspect, 0.01, 200.0);
    }
    glMatrixMode(GL_MODELVIEW);
    wglMakeCurrent(NULL, NULL);
    ReleaseDC(g_hGLWnd, hdc);
    InvalidateRect(g_hGLWnd, NULL, FALSE);
}

void ActionResetAll() {
    g_model.resetVars();
    g_camera.resetVars();
    g_viewMode = VIEW_WIREFRAME;
    g_isOrtho = 1;
    InvalidateRect(g_hGLWnd, NULL, FALSE);
}

void ActionQuit() { PostQuitMessage(0); }

// ---- 同时支持静态 OBJ 与动态 FBX ----
void ActionOpenModel() {
    wchar_t szFile[MAX_PATH] = L"";
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hMainWnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"FBX 动画模型 (*.fbx)\0*.fbx\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrTitle = L"请选择加载外部 3Dfbx 模型资产文件";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    if (GetOpenFileNameW(&ofn)) {
        char narrowPath[MAX_PATH];
        WideCharToMultiByte(CP_UTF8, 0, szFile, -1, narrowPath, MAX_PATH, NULL, NULL);

        // 清除先前显示的显示列表
        if (g_displayList != 0 && g_hGLDC && g_hGLRC) {
            HDC hdc = GetDC(g_hGLWnd);
            wglMakeCurrent(hdc, g_hGLRC);
            glDeleteLists(g_displayList, 1);
            g_displayList = 0;
            wglMakeCurrent(NULL, NULL);
            ReleaseDC(g_hGLWnd, hdc);
        }

        ActionResetAll();
        g_modelLoaded = false;

        // 根据路径末尾的扩展名派发加载处理器
        wstring pathStr(szFile);
        size_t dotIdx = pathStr.find_last_of(L'.');
        wstring ext = (dotIdx != wstring::npos) ? pathStr.substr(dotIdx) : L"";
        for (auto& c : ext) c = towlower(c);

        if (ext == L".fbx") {
            g_modelLoaded = g_model.loadFBX(narrowPath);
        }

        // 强制重绘激活的双面板
        InvalidateRect(g_hGLWnd, NULL, FALSE);
        InvalidateRect(g_hMainWnd, NULL, FALSE);
    }
}

// 初始化 OpenGL 上下文
void InitOpenGL(HWND hWnd) {
    PIXELFORMATDESCRIPTOR pfd = { sizeof(PIXELFORMATDESCRIPTOR), 1, PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, PFD_TYPE_RGBA, 24, 0,0,0,0,0,0,0,0,0,0,0,0,0, 32, 0,0, PFD_MAIN_PLANE, 0,0,0,0 };
    g_hGLDC = GetDC(hWnd);
    int pixelFormat = ChoosePixelFormat(g_hGLDC, &pfd);
    SetPixelFormat(g_hGLDC, pixelFormat, &pfd);
    g_hGLRC = wglCreateContext(g_hGLDC);
    wglMakeCurrent(g_hGLDC, g_hGLRC);

    glClearColor(0.12f, 0.12f, 0.14f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    GLfloat lightPos[] = { 3.0f, 5.0f, 4.0f, 1.0f };
    GLfloat ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    GLfloat diffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);

    if (g_camera.camera_distance < 0.1f) g_camera.camera_distance = 3.0f;
    wglMakeCurrent(NULL, NULL);
}

void ResizeOpenGL(int w, int h) {
    g_glWidth = w; g_glHeight = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspect = (h > 0) ? (float)w / (float)h : 1.0f;
    if (g_isOrtho) {
        float s = g_camera.camera_distance * 0.35f;
        glOrtho(-s * aspect, s * aspect, -s, s, 0.01, 200.0);
    }
    else {
        gluPerspective(45.0, aspect, 0.01, 200.0);
    }
    glMatrixMode(GL_MODELVIEW);
}

// ---- 支持动态 FBX 与普通 OBJ 切换渲染 ----
void DrawOpenGLScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 实时帧率 (FPS) 追踪与面板刷新
    static LARGE_INTEGER s_lastFPSTime = {};
    static int s_fpsFrameCount = 0;
    static LARGE_INTEGER s_fpsFreq = {};
    if (s_fpsFreq.QuadPart == 0)
        QueryPerformanceFrequency(&s_fpsFreq);
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    if (s_lastFPSTime.QuadPart == 0)
        s_lastFPSTime = now;
    s_fpsFrameCount++;
    float fpsElapsed = (float)(now.QuadPart - s_lastFPSTime.QuadPart) / (float)s_fpsFreq.QuadPart;
    if (fpsElapsed >= 0.5f) {
        g_currentFPS = (float)s_fpsFrameCount / fpsElapsed;
        s_fpsFrameCount = 0;
        s_lastFPSTime = now;
        swprintf(g_fpsLabel, 64, L"实时帧率: %.1f FPS", g_currentFPS);
        int boneCount = (g_modelLoaded && g_model.is_fbx) ? (int)g_model.bones.size() : 0;
        swprintf(g_boneCountLabel, 64, L"骨骼数量: %d", boneCount);
        if (g_hMainWnd)
            InvalidateRect(g_hMainWnd, NULL, FALSE);
    }

    glLoadIdentity();

    // 1. 应用相机观测矩阵变换
    glTranslatef(-g_camera.camera_x, -g_camera.camera_y, -g_camera.camera_distance);
    glRotatef(g_camera.camera_rotx, 1.0f, 0.0f, 0.0f);
    glRotatef(g_camera.camera_roty, 0.0f, 1.0f, 0.0f);
    glRotatef(g_camera.camera_rotz, 0.0f, 0.0f, 1.0f);

    // 2. 应用模型变换矩阵
    glTranslatef(g_model.model_x, g_model.model_y, g_model.model_z);
    glRotatef(g_model.model_rotx, 1.0f, 0.0f, 0.0f);
    glRotatef(g_model.model_roty, 0.0f, 1.0f, 0.0f);
    glRotatef(g_model.model_rotz, 0.0f, 0.0f, 1.0f);

    // ????????? / ?? / ????
    if (g_viewMode == VIEW_SKELETON) {
        // ???????????????????????????
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        GLfloat wireMat[] = { 0.3f, 0.35f, 0.4f, 0.35f };
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, wireMat);
        if (g_modelLoaded && g_model.is_fbx) {
            for (size_t f = 0; f < g_model.fbx_faces.size(); f++) {
                const auto& face = g_model.fbx_faces[f];
                if (face.vertexIndices.size() == 3) glBegin(GL_TRIANGLES);
                else if (face.vertexIndices.size() == 4) glBegin(GL_QUADS);
                else glBegin(GL_POLYGON);
                for (size_t v = 0; v < face.vertexIndices.size(); v++) {
                    int idx = face.vertexIndices[v];
                    glNormal3f(g_model.normal_list[3 * idx], g_model.normal_list[3 * idx + 1], g_model.normal_list[3 * idx + 2]);
                    glVertex3f(g_model.vertex_list[3 * idx], g_model.vertex_list[3 * idx + 1], g_model.vertex_list[3 * idx + 2]);
                }
                glEnd();
            }
        }
        glDisable(GL_BLEND);
        DrawSkeleton();
        SwapBuffers(g_hGLDC);
        return;
    }
    glPolygonMode(GL_FRONT_AND_BACK, g_viewMode == VIEW_WIREFRAME ? GL_LINE : GL_FILL);

    GLfloat matColor[] = { 0.7f, 0.75f, 0.8f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, matColor);

    // 3. 多模态资产判断分流绘制
    if (g_modelLoaded) {
        if (g_model.is_fbx) {
            // FBX 动画模型采用多边形面即时刻更新的顶点及法线列表
            for (size_t f = 0; f < g_model.fbx_faces.size(); f++) {
                const auto& face = g_model.fbx_faces[f];
                if (face.vertexIndices.size() == 3) glBegin(GL_TRIANGLES);
                else if (face.vertexIndices.size() == 4) glBegin(GL_QUADS);
                else glBegin(GL_POLYGON);

                for (size_t v = 0; v < face.vertexIndices.size(); v++) {
                    int idx = face.vertexIndices[v];
                    glNormal3f(g_model.normal_list[3 * idx], g_model.normal_list[3 * idx + 1], g_model.normal_list[3 * idx + 2]);
                    glVertex3f(g_model.vertex_list[3 * idx], g_model.vertex_list[3 * idx + 1], g_model.vertex_list[3 * idx + 2]);
                }
                glEnd();
            }
        }
        else {
            // OBJ 静态模型采用局部硬编码的显示列表
            if (g_displayList != 0) glCallList(g_displayList);
        }
    }

    SwapBuffers(g_hGLDC);
}

// 窗口客户区域使用的 Win32 消息处理回调函数
LRESULT CALLBACK OpenGLWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: InitOpenGL(hWnd); return 0;
    case WM_SIZE: {
        int cx = LOWORD(lParam); int cy = HIWORD(lParam);
        HDC hdc = GetDC(hWnd); wglMakeCurrent(hdc, g_hGLRC);
        ResizeOpenGL(cx, cy); wglMakeCurrent(NULL, NULL);
        ReleaseDC(hWnd, hdc);
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC hdc = BeginPaint(hWnd, &ps);
        wglMakeCurrent(g_hGLDC, g_hGLRC);
        DrawOpenGLScene();
        wglMakeCurrent(NULL, NULL);
        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
        if (message == WM_LBUTTONDOWN) g_mouseLDown = true;
        else g_mouseRDown = true;
        g_lastMouseX = LOWORD(lParam); g_lastMouseY = HIWORD(lParam);
        SetCapture(hWnd);
        return 0;
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
        if (message == WM_LBUTTONUP) g_mouseLDown = false;
        else g_mouseRDown = false;
        if (!g_mouseLDown && !g_mouseRDown) ReleaseCapture();
        return 0;
    case WM_MOUSEMOVE: {
        int mx = LOWORD(lParam); int my = HIWORD(lParam);
        int dx = mx - g_lastMouseX; int dy = my - g_lastMouseY;
        if (g_mouseLDown) { // 旋转
            g_camera.camera_roty += dx * 0.4f;
            g_camera.camera_rotx += dy * 0.4f;
            InvalidateRect(hWnd, NULL, FALSE);
        }
        else if (g_mouseRDown) { // 平移
            g_camera.camera_x -= dx * 0.005f * g_camera.camera_distance;
            g_camera.camera_y += dy * 0.005f * g_camera.camera_distance;
            InvalidateRect(hWnd, NULL, FALSE);
        }
        g_lastMouseX = mx; g_lastMouseY = my;
        return 0;
    }
    case WM_MOUSEWHEEL: {
        short zDelta = (short)HIWORD(wParam);
        g_camera.camera_distance -= (float)zDelta * 0.002f;
        if (g_camera.camera_distance < 0.2f) g_camera.camera_distance = 0.2f;
        HDC hdc = GetDC(hWnd); wglMakeCurrent(hdc, g_hGLRC);
        ResizeOpenGL(g_glWidth, g_glHeight); wglMakeCurrent(NULL, NULL); ReleaseDC(hWnd, hdc);
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
    }
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

// 主外壳框架窗口的 Win32 消息回调处理器
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_SIZE: {
        int w = LOWORD(lParam); int h = HIWORD(lParam);
        MoveWindow(g_hGLWnd, 0, 0, w - PANEL_WIDTH, h, TRUE);
        InvalidateRect(hWnd, NULL, TRUE);
        return 0;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC hdc = BeginPaint(hWnd, &ps);
        RECT rect; GetClientRect(hWnd, &rect);
        rect.left = rect.right - PANEL_WIDTH;
        DrawPanel(hdc, rect);
        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_MOUSEMOVE: {
        int mx = LOWORD(lParam); int my = HIWORD(lParam);
        RECT rect; GetClientRect(hWnd, &rect);
        int panelLeft = rect.right - PANEL_WIDTH;
        int oldHover = g_hoverBtn;
        g_hoverBtn = -1;
        if (mx >= panelLeft) {
            int relX = mx - panelLeft;
            int relY = my;
            // ????????
            g_viewModeDropdownHover = -1;
            if (g_viewModeDropdownOpen) {
                for (int i = 0; i < g_btnCount; i++) {
                    if (g_buttons[i].type == BTN_DROPDOWN && g_buttons[i].actionId == AID_VIEW_MODE) {
                        GButton& db = g_buttons[i];
                        int dby = db.rect.top + (db.rect.bottom - db.rect.top);
                        int dbx = db.rect.left + 8;
                        int dbw = (db.rect.right - db.rect.left) - 16;
                        for (int oi = 0; oi < g_viewModeOptionCount; oi++) {
                            int oy = dby + oi * 28;
                            if (relX >= dbx && relX <= dbx + dbw && relY >= oy && relY <= oy + 26)
                                g_viewModeDropdownHover = oi;
                        }
                        break;
                    }
                }
            }
            int idx = HitTestButtons(relX, relY);
            if (idx != -1) {
                g_hoverBtn = idx;
                g_buttons[idx].hover = true;
            }
        }
        for (int i = 0; i < g_btnCount; i++) {
            if (i != g_hoverBtn) g_buttons[i].hover = false;
        }
        if (g_hoverBtn != oldHover) InvalidateRect(hWnd, NULL, FALSE);
        return 0;
    }
    case WM_LBUTTONDOWN: {
        int mx = LOWORD(lParam); int my = HIWORD(lParam);
        RECT rect; GetClientRect(hWnd, &rect);
        int panelLeft = rect.right - PANEL_WIDTH;
        if (mx >= panelLeft) {
            int relX = mx - panelLeft;
            int relY = my;
            if (g_viewModeDropdownOpen) {
                for (int i = 0; i < g_btnCount; i++) {
                    if (g_buttons[i].type == BTN_DROPDOWN && g_buttons[i].actionId == AID_VIEW_MODE) {
                        GButton& db = g_buttons[i];
                        int dby = db.rect.top + (db.rect.bottom - db.rect.top);
                        int dbx = db.rect.left + 8;
                        int dbw = (db.rect.right - db.rect.left) - 16;
                        for (int oi = 0; oi < g_viewModeOptionCount; oi++) {
                            int oy = dby + oi * 28;
                            if (relX >= dbx && relX <= dbx + dbw && relY >= oy && relY <= oy + 26) {
                                g_viewMode = oi;
                                g_viewModeDropdownOpen = false;
                                g_viewModeDropdownHover = -1;
                                InvalidateRect(g_hGLWnd, NULL, FALSE);
                                InvalidateRect(hWnd, NULL, FALSE);
                                return 0;
                            }
                        }
                        break;
                    }
                }
                // ??????????????
                g_viewModeDropdownOpen = false;
                g_viewModeDropdownHover = -1;
                InvalidateRect(hWnd, NULL, FALSE);
                return 0;
            }
            int idx = HitTestButtons(relX, relY);
            if (idx != -1) {
                g_buttons[idx].pressed = true;
                InvalidateRect(hWnd, NULL, FALSE);
                ExecuteAction(g_buttons[idx].actionId);
            }
        }
        return 0;
    }
    case WM_LBUTTONUP: {
        for (int i = 0; i < g_btnCount; i++) g_buttons[i].pressed = false;
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
    }
    case WM_KEYDOWN: {
        if (wParam == VK_SPACE) ActionCycleViewMode();
        else if (wParam == 'V') ActionToggleProjection();
        else if (wParam == 'S') ActionResetAll();
        else if (wParam == 'Q') ActionQuit();
        else if (wParam == 'O') ActionOpenModel();
        else if (wParam == 'P') { g_animPlaying = !g_animPlaying; InvalidateRect(hWnd, NULL, FALSE); }
        return 0;
    }
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

// ============================================================
// 程序入口函数 WinMain
// ============================================================
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    GdiplusStartup(&g_gdiToken, &g_gdiStartupInput, NULL);
    InitPanelButtons();

    WNDCLASS wc = {};
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = L"ModelViewerMainShell";
    if (!RegisterClass(&wc)) return 1;

    WNDCLASS glwc = {};
    glwc.lpfnWndProc = OpenGLWndProc;
    glwc.hInstance = hInstance;
    glwc.hCursor = LoadCursor(NULL, IDC_ARROW);
    glwc.lpszClassName = L"ModelViewerGLView";
    glwc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    if (!RegisterClass(&glwc)) return 1;

    int winW = 1250; int winH = 800;
    g_hMainWnd = CreateWindowEx(0, L"ModelViewerMainShell", L"高级3D模型查看器(FBX Skeletal Anim + OBJ Static)", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, winW, winH, NULL, NULL, hInstance, NULL);
    if (!g_hMainWnd) return 1;

    g_hGLWnd = CreateWindowEx(0, L"ModelViewerGLView", L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, 0, 0, winW - PANEL_WIDTH, winH, g_hMainWnd, NULL, hInstance, NULL);
    if (!g_hGLWnd) return 1;

    ShowWindow(g_hMainWnd, nCmdShow);
    UpdateWindow(g_hMainWnd);

    // 初始化高性能渲染循环
    LARGE_INTEGER frequency, lastTime, currentTime;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&lastTime);

    MSG msg = {};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            // 当激活 FBX 资产时，在空闲周期执行高精度的 CPU 蒙皮骨骼重算并刷新视口
            if (g_modelLoaded && g_model.is_fbx && g_animPlaying) {
                QueryPerformanceCounter(&currentTime);
                float deltaTime = (float)(currentTime.QuadPart - lastTime.QuadPart) / frequency.QuadPart;
                lastTime = currentTime;

                if (deltaTime > 0.1f) deltaTime = 0.1f; // 拦截断点挂起引发的过长跳帧

                g_model.updateAnimation(deltaTime);

                // 仅刷新 OpenGL 视口，绝不盲目刷 UI 面板，防止发生高频闪烁
                InvalidateRect(g_hGLWnd, NULL, FALSE);
                UpdateWindow(g_hGLWnd);
            }
            else {
                Sleep(16); // 静态模型状态下切回 60FPS 基础抗锯齿空闲休眠，彻底释放 CPU 核心空转开销
                QueryPerformanceCounter(&lastTime);
            }
        }
    }

    GdiplusShutdown(g_gdiToken);
    return (int)msg.wParam;
}
