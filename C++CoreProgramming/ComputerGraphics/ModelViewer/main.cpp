// ============================================================
// ModelViewer - Win32 OpenGL + GDI+ Interactive Panel
// Converted from GLUT to Windows native application
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
// Dark Gray Color Palette
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
#define BTN_SMALL_HEIGHT      28
#define SECTION_GAP           6
#define PANEL_LEFT_PAD        10

// ============================================================
// Forward declarations
// ============================================================
LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK OpenGLWndProc(HWND, UINT, WPARAM, LPARAM);
void InitOpenGL(HWND hWnd);
void ResizeOpenGL(int w, int h);
void DrawOpenGLScene();
void BuildDisplayList();
void LoadOBJFile(const char* filename);
void ActionToggleWireframe();

void ActionToggleProjection();
void ActionModelTransX(int dir);
void ActionModelTransY(int dir);
void ActionModelTransZ(int dir);
void ActionModelRotX(int dir);
void ActionModelRotY(int dir);
void ActionModelRotZ(int dir);
void ActionCameraTransX(int dir);
void ActionCameraTransY(int dir);
void ActionCameraTransZ(int dir);
void ActionCameraRotX(int dir);
void ActionCameraRotY(int dir);
void ActionCameraRotZ(int dir);
void ActionSaveModel();
void ActionResetAll();
void ActionQuit();
void ActionOpenModel();

// ============================================================
// Button types for the GDI+ panel
// ============================================================
enum BtnType {
    BTN_TOGGLE,      // toggle button (wireframe, projection)
    BTN_DIRECTION,   // directional button pair (left/right, up/down)
    BTN_ACTION,      // single action button
    BTN_HEADER,      // section header (non-clickable)
    BTN_INFO         // info display only
};

struct GButton {
    RECT rect;
    BtnType type;
    const wchar_t* label;
    const wchar_t* keyText;
    const wchar_t* keyText2;    // second key for directional buttons
    int actionId;               // action identifier
    bool hover;
    bool pressed;
    int hoverSub;               // for directional: -1=left/down, 0=none, 1=right/up
};

enum ActionId {
    AID_TOGGLE_WIREFRAME = 1,
    AID_TOGGLE_FOG,
    AID_TOGGLE_PROJECTION,
    AID_MODEL_TRANS_X_NEG,  AID_MODEL_TRANS_X_POS,
    AID_MODEL_TRANS_Y_NEG,  AID_MODEL_TRANS_Y_POS,
    AID_MODEL_TRANS_Z_NEG,  AID_MODEL_TRANS_Z_POS,
    AID_MODEL_ROT_X_NEG,    AID_MODEL_ROT_X_POS,
    AID_MODEL_ROT_Y_NEG,    AID_MODEL_ROT_Y_POS,
    AID_MODEL_ROT_Z_NEG,    AID_MODEL_ROT_Z_POS,
    AID_CAMERA_TRANS_X_NEG, AID_CAMERA_TRANS_X_POS,
    AID_CAMERA_TRANS_Y_NEG, AID_CAMERA_TRANS_Y_POS,
    AID_CAMERA_TRANS_Z_NEG, AID_CAMERA_TRANS_Z_POS,
    AID_CAMERA_ROT_X_NEG,   AID_CAMERA_ROT_X_POS,
    AID_CAMERA_ROT_Y_NEG,   AID_CAMERA_ROT_Y_POS,
    AID_CAMERA_ROT_Z_NEG,   AID_CAMERA_ROT_Z_POS,
    AID_SAVE_MODEL,
    AID_RESET_ALL,
    AID_QUIT,
    AID_OPEN_MODEL
};

// ============================================================
// Global state
// ============================================================
static HWND g_hMainWnd = NULL;
static HWND g_hGLWnd = NULL;
static HDC g_hGLDC = NULL;
static HGLRC g_hGLRC = NULL;
static int g_glWidth = 0, g_glHeight = 0;

static Model    g_model;
static Camera   g_camera;
static GLuint   g_displayList = 0;

static int g_isWire = 1;
static int g_isOrtho = 1;
static bool g_modelLoaded = false;

// Mouse interaction state
static bool  g_mouseLDown = false;
static bool  g_mouseRDown = false;
static int   g_lastMouseX = 0;
static int   g_lastMouseY = 0;

static GButton g_buttons[64];
static int g_btnCount = 0;
static int g_panelScrollOffset = 0;
static int g_panelContentHeight = 0;
static int g_hoverBtn = -1;

static GdiplusStartupInput g_gdiStartupInput;
static ULONG_PTR g_gdiToken = 0;

// ============================================================
// Helper: Create a GButton
// ============================================================
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

// ============================================================
// Initialize all buttons for the right panel
// ============================================================
void InitPanelButtons() {
    g_btnCount = 0;
    int y = PANEL_PADDING + 8;

    // ---- Title ----
    AddButton(BTN_HEADER, L"3D MODEL VIEWER", L"", L"", 0, y, 36);
    y += 42;

    // ---- File ----
    AddButton(BTN_HEADER, L"▸ 文件", L"", L"", 0, y, 26);
    y += 30;
    AddButton(BTN_ACTION, L"打开 Wavefront OBJ 模型...", L"O", L"", AID_OPEN_MODEL, y);
    y += BTN_HEIGHT + SECTION_GAP + 4;

    // ---- Display Mode ----
    AddButton(BTN_HEADER, L"▸ 显示模式", L"", L"", 0, y, 26);
    y += 30;
    AddButton(BTN_TOGGLE, L"线框 / 填充", L"Space", L"", AID_TOGGLE_WIREFRAME, y);
    y += BTN_HEIGHT + 4;

    AddButton(BTN_TOGGLE, L"正交 / 透视投影", L"V", L"", AID_TOGGLE_PROJECTION, y);
    y += BTN_HEIGHT + SECTION_GAP + 4;

    // ---- Actions ----
    AddButton(BTN_HEADER, L"▸ 操作", L"", L"", 0, y, 26);
    y += 30;
    AddButton(BTN_ACTION, L"重置视角", L"S", L"", AID_RESET_ALL, y);
    y += BTN_HEIGHT + 4;
    AddButton(BTN_ACTION, L"退出程序", L"Q", L"", AID_QUIT, y);
    y += BTN_HEIGHT + SECTION_GAP + 4;

    // ---- Mouse controls info ----
    AddButton(BTN_HEADER, L"▸ 鼠标操作", L"", L"", 0, y, 26);
    y += 30;
    AddButton(BTN_INFO, L"左键拖拽  —  旋转视角", L"", L"", 0, y, 28);
    y += 28 + 3;
    AddButton(BTN_INFO, L"右键拖拽  —  平移视角", L"", L"", 0, y, 28);
    y += 28 + 3;
    AddButton(BTN_INFO, L"滚轮  —  缩放", L"", L"", 0, y, 28);
    y += 28 + PANEL_PADDING;

    g_panelContentHeight = y;
}

// ============================================================
// GDI+ Drawing helpers
// ============================================================
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

Color ToGdiColor(COLORREF c, BYTE a = 255) {
    return Color(a, GetRValue(c), GetGValue(c), GetBValue(c));
}

// ============================================================
// Draw the right panel entirely with GDI+
// ============================================================
void DrawPanel(HDC hdc, RECT& panelRect) {
    int pw = panelRect.right - panelRect.left;
    int ph = panelRect.bottom - panelRect.top;

    // Create a backbuffer bitmap for flicker-free drawing
    Bitmap backBuffer(pw, ph);
    Graphics g(&backBuffer);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintAntiAlias);

    // --- Panel background ---
    SolidBrush bgBrush(ToGdiColor(CLR_BG_PANEL));
    g.FillRectangle(&bgBrush, 0, 0, pw, ph);

    // --- Left border line ---
    Pen borderPen(ToGdiColor(CLR_BORDER), 1.0f);
    g.DrawLine(&borderPen, 0, 0, 0, ph);

    // --- Title bar ---
    SolidBrush titleBg(ToGdiColor(CLR_BG_HEADER));
    g.FillRectangle(&titleBg, 0, 0, pw, 50);


    // --- Separator under title ---
    Pen sepPen(ToGdiColor(CLR_SEPARATOR), 1.0f);
    g.DrawLine(&sepPen, 0, 50, pw, 50);

    // --- Draw buttons ---
    Font sectionFont(L"Segoe UI", 9, FontStyleBold);
    Font btnFont(L"Segoe UI", 9);
    Font keyFont(L"Segoe UI", 9, FontStyleBold);
    Font keySmallFont(L"Segoe UI", 8, FontStyleBold);

    SolidBrush sectionBrush(ToGdiColor(CLR_TEXT_DIM));
    SolidBrush btnTextBrush(ToGdiColor(CLR_TEXT));
    SolidBrush keyBrush(ToGdiColor(CLR_TEXT_HEADER));
    SolidBrush keyOffBrush(ToGdiColor(CLR_TEXT_DIM));
    SolidBrush toggleOnBrush(ToGdiColor(CLR_TOGGLE_ON));
    SolidBrush toggleOffBrush(ToGdiColor(CLR_TOGGLE_OFF));

    Pen btnBorder(ToGdiColor(CLR_BORDER), 1.0f);

    for (int i = 0; i < g_btnCount; i++) {
        GButton& b = g_buttons[i];
        int bx = b.rect.left;
        int by = b.rect.top - g_panelScrollOffset;
        int bw = b.rect.right - b.rect.left;
        int bh = b.rect.bottom - b.rect.top;

        // Skip if outside visible area
        if (by + bh < 50 || by > ph) continue;

        if (b.type == BTN_HEADER) {
            g.DrawString(b.label, -1, &sectionFont, PointF((float)bx + 2, (float)by + 4), &sectionBrush);
            continue;
        }

        // Button background
        Color btnBgClr = b.hover ? ToGdiColor(CLR_BG_BUTTON_HOVER) : ToGdiColor(CLR_BG_BUTTON);
        if (b.pressed) btnBgClr = ToGdiColor(CLR_BG_BUTTON_ACTIVE);
        SolidBrush btnBg(btnBgClr);
        DrawRoundRect(g, &btnBorder, &btnBg, bx, by, bw, bh, 5);

        int textX = bx + 10;
        int textY = by + (bh - 16) / 2;

        if (b.type == BTN_TOGGLE) {
            g.DrawString(b.label, -1, &btnFont, PointF((float)textX, (float)textY), &btnTextBrush);

            // Draw toggle state indicator
            bool state = false;
            if (b.actionId == AID_TOGGLE_WIREFRAME) state = !g_isWire;  // fill mode
            else if (b.actionId == AID_TOGGLE_PROJECTION) state = !g_isOrtho; // perspective

            int indX = bx + bw - 55;
            int indY = by + (bh - 14) / 2;
            SolidBrush* indBrush = state ? &toggleOnBrush : &toggleOffBrush;
            DrawRoundRect(g, NULL, indBrush, indX, indY, 44, 14, 7);
            Font indFont(L"Segoe UI", 7, FontStyleBold);
            SolidBrush indText(Color(255, 255, 255, 255));
            g.DrawString(state ? L"ON" : L"OFF", -1, &indFont, PointF((float)indX + (state ? 14 : 10), (float)indY + 1), &indText);

            // Key badge
            int kx = bx + bw - 92;
            int ky = by + (bh - 18) / 2;
            SolidBrush keyBg(ToGdiColor(CLR_BG_KEY));
            DrawRoundRect(g, NULL, &keyBg, kx, ky, 32, 18, 4);
            g.DrawString(b.keyText, -1, &keySmallFont, PointF((float)kx + 6, (float)ky + 3), &keyBrush);

        } else if (b.type == BTN_DIRECTION) {
            // Label
            g.DrawString(b.label, -1, &btnFont, PointF((float)textX, (float)textY), &btnTextBrush);

            // Two directional buttons on the right
            int dirBtnW = 36;
            int dirBtnH = bh - 6;
            int dirBtnY = by + 3;

            // Left / Negative button
            int negX = bx + bw - dirBtnW * 2 - 16;
            bool negHover = (b.hover && b.hoverSub == -1);
            Color negClr = negHover ? ToGdiColor(CLR_BG_BUTTON_HOVER) : ToGdiColor(CLR_BG_KEY);
            SolidBrush negBg(negClr);
            DrawRoundRect(g, NULL, &negBg, negX, dirBtnY, dirBtnW, dirBtnH, 4);
            g.DrawString(b.keyText2, -1, &keyFont, PointF((float)negX + 10, (float)dirBtnY + (dirBtnH - 16) / 2), &keyOffBrush);

            // Right / Positive button
            int posX = negX + dirBtnW + 4;
            bool posHover = (b.hover && b.hoverSub == 1);
            Color posClr = posHover ? ToGdiColor(CLR_BG_BUTTON_HOVER) : ToGdiColor(CLR_BG_KEY);
            SolidBrush posBg(posClr);
            DrawRoundRect(g, NULL, &posBg, posX, dirBtnY, dirBtnW, dirBtnH, 4);
            g.DrawString(b.keyText, -1, &keyFont, PointF((float)posX + 10, (float)dirBtnY + (dirBtnH - 16) / 2), &keyBrush);

        } else if (b.type == BTN_ACTION) {
            g.DrawString(b.label, -1, &btnFont, PointF((float)textX, (float)textY), &btnTextBrush);

            // Key badge (Quit uses red-ish key text)
            int kx = bx + bw - 46;
            int ky = by + (bh - 18) / 2;
            SolidBrush keyBg(ToGdiColor(CLR_BG_KEY));
            DrawRoundRect(g, NULL, &keyBg, kx, ky, 32, 18, 4);
            SolidBrush quitKeyBrush(Color(255, 220, 100, 100));
            SolidBrush* kptr = (b.actionId == AID_QUIT) ? &quitKeyBrush : &keyBrush;
            g.DrawString(b.keyText, -1, &keySmallFont, PointF((float)kx + 8, (float)ky + 3), kptr);

        } else if (b.type == BTN_INFO) {
            g.DrawString(b.label, -1, &btnFont, PointF((float)textX, (float)textY), &sectionBrush);
        }
    }

    // Blit backbuffer to screen
    Graphics screen(hdc);
    screen.DrawImage(&backBuffer, (INT)panelRect.left, (INT)panelRect.top, 0, 0, pw, ph, UnitPixel);
}

// ============================================================
// Button hit testing
// ============================================================
int HitTestButtons(int mx, int my) {
    for (int i = 0; i < g_btnCount; i++) {
        GButton& b = g_buttons[i];
        if (b.type == BTN_HEADER || b.type == BTN_INFO) continue;
        RECT r = b.rect;
        r.top -= g_panelScrollOffset;
        r.bottom -= g_panelScrollOffset;
        if (mx >= r.left && mx <= r.right && my >= r.top && my <= r.bottom) {
            return i;
        }
    }
    return -1;
}

int HitTestDirSub(GButton& b, int mx, int my) {
    if (b.type != BTN_DIRECTION) return 0;
    int bw = b.rect.right - b.rect.left;
    int dirBtnW = 36;
    int negX = b.rect.left + bw - dirBtnW * 2 - 16;
    int posX = negX + dirBtnW + 4;
    int dirBtnY = b.rect.top - g_panelScrollOffset + 3;
    int dirBtnH = (b.rect.bottom - b.rect.top) - 6;

    if (mx >= negX && mx <= negX + dirBtnW && my >= dirBtnY && my <= dirBtnY + dirBtnH)
        return -1;
    if (mx >= posX && mx <= posX + dirBtnW && my >= dirBtnY && my <= dirBtnY + dirBtnH)
        return 1;
    return 0;
}

// ============================================================
// Execute button action
// ============================================================
void ExecuteAction(int actionId, int sub) {
    switch (actionId) {
    case AID_TOGGLE_WIREFRAME: ActionToggleWireframe(); break;

    case AID_TOGGLE_PROJECTION:ActionToggleProjection(); break;
    case AID_MODEL_TRANS_X_POS: ActionModelTransX(sub); break;
    case AID_MODEL_TRANS_Y_POS: ActionModelTransY(sub); break;
    case AID_MODEL_TRANS_Z_POS: ActionModelTransZ(sub); break;
    case AID_MODEL_ROT_X_POS:   ActionModelRotX(sub); break;
    case AID_MODEL_ROT_Y_POS:   ActionModelRotY(sub); break;
    case AID_MODEL_ROT_Z_POS:   ActionModelRotZ(sub); break;
    case AID_CAMERA_TRANS_X_POS:ActionCameraTransX(sub); break;
    case AID_CAMERA_TRANS_Y_POS:ActionCameraTransY(sub); break;
    case AID_CAMERA_TRANS_Z_POS:ActionCameraTransZ(sub); break;
    case AID_CAMERA_ROT_X_POS:  ActionCameraRotX(sub); break;
    case AID_CAMERA_ROT_Y_POS:  ActionCameraRotY(sub); break;
    case AID_CAMERA_ROT_Z_POS:  ActionCameraRotZ(sub); break;
    case AID_SAVE_MODEL:  ActionSaveModel(); break;
    case AID_RESET_ALL:   ActionResetAll(); break;
    case AID_QUIT:        ActionQuit(); break;
    case AID_OPEN_MODEL:  ActionOpenModel(); break;
    }
}

// ============================================================
// Action Implementations
// ============================================================
void ActionToggleWireframe() {
    g_isWire = !g_isWire;
    InvalidateRect(g_hGLWnd, NULL, FALSE);
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
    } else {
        gluPerspective(45.0, aspect, 0.01, 200.0);
    }
    glMatrixMode(GL_MODELVIEW);
    wglMakeCurrent(NULL, NULL);
    ReleaseDC(g_hGLWnd, hdc);
    InvalidateRect(g_hGLWnd, NULL, FALSE);
}

void ActionModelTransX(int dir) {
    g_model.model_x += dir * 0.1f;
    InvalidateRect(g_hGLWnd, NULL, FALSE);
}
void ActionModelTransY(int dir) {
    g_model.model_y += dir * 0.1f;
    InvalidateRect(g_hGLWnd, NULL, FALSE);
}
void ActionModelTransZ(int dir) {
    g_model.model_z += dir * 0.1f;
    InvalidateRect(g_hGLWnd, NULL, FALSE);
}
void ActionModelRotX(int dir) {
    g_model.model_rotx += dir * 10;
    if (g_model.model_rotx < 0) g_model.model_rotx += 360;
    if (g_model.model_rotx >= 360) g_model.model_rotx -= 360;
    InvalidateRect(g_hGLWnd, NULL, FALSE);
}
void ActionModelRotY(int dir) {
    g_model.model_roty += dir * 10;
    if (g_model.model_roty < 0) g_model.model_roty += 360;
    if (g_model.model_roty >= 360) g_model.model_roty -= 360;
    InvalidateRect(g_hGLWnd, NULL, FALSE);
}
void ActionModelRotZ(int dir) {
    g_model.model_rotz += dir * 10;
    if (g_model.model_rotz < 0) g_model.model_rotz += 360;
    if (g_model.model_rotz >= 360) g_model.model_rotz -= 360;
    InvalidateRect(g_hGLWnd, NULL, FALSE);
}
void ActionCameraTransX(int dir) {
    g_camera.camera_x += dir * 0.1f;
    InvalidateRect(g_hGLWnd, NULL, FALSE);
}
void ActionCameraTransY(int dir) {
    g_camera.camera_y += dir * 0.1f;
    InvalidateRect(g_hGLWnd, NULL, FALSE);
}
void ActionCameraTransZ(int dir) {
    g_camera.camera_distance -= dir * 0.15f;
    if (g_camera.camera_distance < 0.5f)  g_camera.camera_distance = 0.5f;
    if (g_camera.camera_distance > 50.0f) g_camera.camera_distance = 50.0f;
    InvalidateRect(g_hGLWnd, NULL, FALSE);
}
void ActionCameraRotX(int dir) {
    g_camera.camera_rotx += dir * 10;
    if (g_camera.camera_rotx >= 360) g_camera.camera_rotx -= 360;
    if (g_camera.camera_rotx < 0) g_camera.camera_rotx += 360;
    InvalidateRect(g_hGLWnd, NULL, FALSE);
}
void ActionCameraRotY(int dir) {
    g_camera.camera_roty += dir * 10;
    if (g_camera.camera_roty >= 360) g_camera.camera_roty -= 360;
    if (g_camera.camera_roty < 0) g_camera.camera_roty += 360;
    InvalidateRect(g_hGLWnd, NULL, FALSE);
}
void ActionCameraRotZ(int dir) {
    g_camera.camera_rotz += dir * 10;
    if (g_camera.camera_rotz >= 360) g_camera.camera_rotz -= 360;
    if (g_camera.camera_rotz < 0) g_camera.camera_rotz += 360;
    InvalidateRect(g_hGLWnd, NULL, FALSE);
}

void ActionSaveModel() {
    g_model.saveModel();
    MessageBox(g_hMainWnd, L"模型已保存到 output.obj", L"保存成功", MB_OK | MB_ICONINFORMATION);
}

void ActionResetAll() {
    g_model.resetVars();
    g_camera.resetVars();
    g_isWire = 1;
    g_isOrtho = 1;
    InvalidateRect(g_hGLWnd, NULL, FALSE);
}

void ActionQuit() {
    PostQuitMessage(0);
}

void ActionOpenModel() {
    // Configure OPENFILENAME for .obj file selection
    wchar_t szFile[MAX_PATH] = L"";
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hMainWnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"Wavefront OBJ Files (*.obj)\0*.obj\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrTitle = L"打开 Wavefront OBJ 模型文件";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    if (GetOpenFileNameW(&ofn)) {
        // Convert wide string to narrow for LoadOBJFile
        char narrowPath[MAX_PATH];
        WideCharToMultiByte(CP_UTF8, 0, szFile, -1, narrowPath, MAX_PATH, NULL, NULL);

        // Reset state before loading new model
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
        g_model.vertex_list.clear();
        g_model.s_list.clear();

        // Load the selected OBJ file
        LoadOBJFile(narrowPath);

        // Force immediate repaint of both panels
        InvalidateRect(g_hGLWnd, NULL, FALSE);
        UpdateWindow(g_hGLWnd);
        InvalidateRect(g_hMainWnd, NULL, FALSE);
        UpdateWindow(g_hMainWnd);
    }
}

// ============================================================
// OpenGL: Load OBJ file and build display list
// ============================================================
void LoadOBJFile(const char* filename) {
    g_model.vertex_list.clear();
    g_model.s_list.clear();

    ifstream fileObject(filename);
    if (!fileObject.is_open()) {
        wchar_t buf[512];
        swprintf(buf, 512, L"无法打开文件:\n%hs", filename);
        MessageBox(NULL, buf, L"错误", MB_OK | MB_ICONERROR);
        return;
    }

    string fileObjectLine;

    // First pass: read vertices
    while (getline(fileObject, fileObjectLine)) {
        if (!fileObjectLine.empty() && fileObjectLine[0] == 'v' && fileObjectLine[1] == ' ') {
            float x, y, z;
            fileObjectLine[0] = ' ';
            sscanf_s(fileObjectLine.c_str(), "%f %f %f", &x, &y, &z);
            g_model.vertex_list.push_back(x);
            g_model.vertex_list.push_back(y);
            g_model.vertex_list.push_back(z);
        }
    }

    // Apply normalization transformations
    g_model.applyTransfToMatrix();

    // Second pass: read faces
    fileObject.clear();
    fileObject.seekg(0, ios::beg);

    while (getline(fileObject, fileObjectLine)) {
        if (!fileObjectLine.empty() && fileObjectLine[0] == 'f' && fileObjectLine[1] == ' ') {
            g_model.s_list.push_back(fileObjectLine);
        }
    }

    fileObject.close();
    g_modelLoaded = true;

    // Build the OpenGL display list
    BuildDisplayList();
}

void BuildDisplayList() {
    if (!g_hGLDC || !g_hGLRC) return;

    HDC hdc = GetDC(g_hGLWnd);
    wglMakeCurrent(hdc, g_hGLRC);

    if (g_displayList != 0) {
        glDeleteLists(g_displayList, 1);
    }

    g_displayList = glGenLists(1);
    glNewList(g_displayList, GL_COMPILE);

    for (size_t h = 0; h < g_model.s_list.size(); h++) {
        string line = g_model.s_list[h];
        line[0] = ' ';
        istringstream iss(line);

        // Collect vertex indices for this polygon
        vector<int> indices;
        while (iss) {
            int value;
            iss >> value;
            if (iss.fail()) break;
            int idx = 3 * (value - 1);
            if (idx + 2 < (int)g_model.vertex_list.size()) {
                indices.push_back(idx);
            }
        }

        if (indices.size() < 3) continue;

        // Compute face normal using Newell's method
        float nx = 0, ny = 0, nz = 0;
        for (size_t i = 0; i < indices.size(); i++) {
            size_t j = (i + 1) % indices.size();
            int a = indices[i], b = indices[j];
            float ax = g_model.vertex_list[a], ay = g_model.vertex_list[a + 1], az = g_model.vertex_list[a + 2];
            float bx = g_model.vertex_list[b], by = g_model.vertex_list[b + 1], bz = g_model.vertex_list[b + 2];
            nx += (ay - by) * (az + bz);
            ny += (az - bz) * (ax + bx);
            nz += (ax - bx) * (ay + by);
        }
        float len = sqrtf(nx * nx + ny * ny + nz * nz);
        if (len > 0.0001f) { nx /= len; ny /= len; nz /= len; }

        glBegin(GL_POLYGON);
        glNormal3f(nx, ny, nz);
        for (size_t i = 0; i < indices.size(); i++) {
            int idx = indices[i];
            glVertex3f(g_model.vertex_list[idx],
                g_model.vertex_list[idx + 1],
                g_model.vertex_list[idx + 2]);
        }
        glEnd();
    }

    glEndList();
    wglMakeCurrent(NULL, NULL);
    ReleaseDC(g_hGLWnd, hdc);
}

// ============================================================
// OpenGL: Initialize
// ============================================================
void InitOpenGL(HWND hWnd) {
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,
        24,     // color depth
        0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0,
        32,     // depth buffer
        0, 0,
        PFD_MAIN_PLANE,
        0, 0, 0, 0
    };

    g_hGLDC = GetDC(hWnd);
    int pixelFormat = ChoosePixelFormat(g_hGLDC, &pfd);
    SetPixelFormat(g_hGLDC, pixelFormat, &pfd);
    g_hGLRC = wglCreateContext(g_hGLDC);
    wglMakeCurrent(g_hGLDC, g_hGLRC);

    glClearColor(0.12f, 0.12f, 0.14f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    // Ensure camera has sane defaults (static globals are zero-initialized!)
    if (g_camera.camera_distance < 0.1f)
        g_camera.camera_distance = 3.0f;

    // ---- Lighting setup ----
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);    // fill light from opposite side
    glEnable(GL_NORMALIZE);

    // Key light (from upper-right-front)
    GLfloat keyAmbient[]  = { 0.12f, 0.12f, 0.14f, 1.0f };
    GLfloat keyDiffuse[]  = { 0.7f, 0.7f, 0.7f, 1.0f };
    GLfloat keySpecular[] = { 0.4f, 0.4f, 0.4f, 1.0f };
    GLfloat keyPos[]      = { 5.0f, 8.0f, 5.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT,  keyAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  keyDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, keySpecular);
    glLightfv(GL_LIGHT0, GL_POSITION, keyPos);

    // Fill light (from lower-left-back, dimmer, no specular)
    GLfloat fillAmbient[]  = { 0.08f, 0.08f, 0.10f, 1.0f };
    GLfloat fillDiffuse[]  = { 0.30f, 0.30f, 0.35f, 1.0f };
    GLfloat fillSpecular[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    GLfloat fillPos[]      = { -4.0f, -1.0f, -4.0f, 1.0f };
    glLightfv(GL_LIGHT1, GL_AMBIENT,  fillAmbient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE,  fillDiffuse);
    glLightfv(GL_LIGHT1, GL_SPECULAR, fillSpecular);
    glLightfv(GL_LIGHT1, GL_POSITION, fillPos);

    // Material
    GLfloat matAmbient[]  = { 0.25f, 0.35f, 0.5f, 1.0f };
    GLfloat matDiffuse[]  = { 0.4f, 0.55f, 0.75f, 1.0f };
    GLfloat matSpecular[] = { 0.6f, 0.6f, 0.6f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,   matAmbient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,   matDiffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  matSpecular);
    glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, 60.0f);

    glShadeModel(GL_SMOOTH);

    // Rebuild display list if model is loaded
    if (g_modelLoaded) {
        BuildDisplayList();
    }

    wglMakeCurrent(NULL, NULL);
    ReleaseDC(hWnd, g_hGLDC);
}

void ResizeOpenGL(int w, int h) {
    if (h == 0) h = 1;
    g_glWidth = w;
    g_glHeight = h;

    HDC hdc = GetDC(g_hGLWnd);
    wglMakeCurrent(hdc, g_hGLRC);

    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    float aspect = (float)w / (float)h;
    if (g_isOrtho) {
        float s = g_camera.camera_distance * 0.35f;
        glOrtho(-s * aspect, s * aspect, -s, s, 0.01, 200.0);
    } else {
        gluPerspective(45.0, aspect, 0.01, 200.0);
    }

    glMatrixMode(GL_MODELVIEW);
    wglMakeCurrent(NULL, NULL);
    ReleaseDC(g_hGLWnd, hdc);
}

void DrawOpenGLScene() {
    if (!g_hGLDC || !g_hGLRC) return;

    HDC hdc = GetDC(g_hGLWnd);
    wglMakeCurrent(hdc, g_hGLRC);

    // ---- Projection matrix (set EVERY frame to guarantee correct state) ----
    float aspect = (g_glHeight > 0) ? (float)g_glWidth / (float)g_glHeight : 1.0f;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (g_isOrtho) {
        float s = g_camera.camera_distance * 0.35f;
        if (s < 0.01f) s = 0.01f;  // safety: prevent zero-sized view
        glOrtho(-s * aspect, s * aspect, -s, s, 0.01, 200.0);
    } else {
        gluPerspective(45.0, aspect, 0.01, 200.0);
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Wireframe
    if (g_isWire)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // ---- Orbit camera via gluLookAt ----
    // Model bounding-box center (pan moves the model with it)
    float ocx = g_model.model_x;
    float ocy = g_model.model_y;
    float ocz = -2.0f + g_model.model_z;

    // Camera orbits around model center, always looks at it
    float rx = g_camera.camera_rotx * 3.14159f / 180.0f;
    float ry = g_camera.camera_roty * 3.14159f / 180.0f;
    float d  = g_camera.camera_distance;

    float eyeX = ocx + d * sinf(ry) * cosf(rx);
    float eyeY = ocy + d * sinf(rx);
    float eyeZ = ocz + d * cosf(ry) * cosf(rx);

    gluLookAt(eyeX, eyeY, eyeZ,   ocx, ocy, ocz,   0.0f, 1.0f, 0.0f);

    // ---- Light position in eye space ----
    GLfloat lightPos[] = { 5.0f, 8.0f, 5.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    // ---- Model rotation about its own center ----
    glTranslatef(ocx, ocy, ocz);
    glRotatef(g_model.model_rotx, 1, 0, 0);
    glRotatef(g_model.model_roty, 0, 1, 0);
    glRotatef(g_model.model_rotz, 0, 0, 1);
    glTranslatef(-ocx, -ocy, -ocz);

    // Draw model
    if (g_displayList != 0) {
        glCallList(g_displayList);
    }

    SwapBuffers(hdc);
    wglMakeCurrent(NULL, NULL);
    ReleaseDC(g_hGLWnd, hdc);
}

// ============================================================
// OpenGL Child Window Proc
// ============================================================
LRESULT CALLBACK OpenGLWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        InitOpenGL(hWnd);
        return 0;

    case WM_SIZE:
        ResizeOpenGL(LOWORD(lParam), HIWORD(lParam));
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        BeginPaint(hWnd, &ps);
        DrawOpenGLScene();
        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_LBUTTONDOWN:
        SetCapture(hWnd);
        g_mouseLDown = true;
        g_lastMouseX = LOWORD(lParam);
        g_lastMouseY = HIWORD(lParam);
        return 0;

    case WM_LBUTTONUP:
        ReleaseCapture();
        g_mouseLDown = false;
        return 0;

    case WM_RBUTTONDOWN:
        SetCapture(hWnd);
        g_mouseRDown = true;
        g_lastMouseX = LOWORD(lParam);
        g_lastMouseY = HIWORD(lParam);
        return 0;

    case WM_RBUTTONUP:
        ReleaseCapture();
        g_mouseRDown = false;
        return 0;

    case WM_MOUSEMOVE: {
        int mx = LOWORD(lParam);
        int my = HIWORD(lParam);
        int dx = mx - g_lastMouseX;
        int dy = my - g_lastMouseY;
        g_lastMouseX = mx;
        g_lastMouseY = my;

        if (g_mouseLDown) {
            // Left drag: orbit camera around model
            // Invert horizontal for natural "drag-the-object" feel
            g_camera.camera_roty -= dx * 0.3f;
            g_camera.camera_rotx += dy * 0.3f;
            // Clamp pitch to prevent pole-crossing flip
            if (g_camera.camera_rotx > 85.0f)  g_camera.camera_rotx = 85.0f;
            if (g_camera.camera_rotx < -85.0f) g_camera.camera_rotx = -85.0f;
            // Wrap yaw
            if (g_camera.camera_roty > 360.0f)  g_camera.camera_roty -= 360.0f;
            if (g_camera.camera_roty < 0.0f)    g_camera.camera_roty += 360.0f;
            InvalidateRect(hWnd, NULL, FALSE);
        } else if (g_mouseRDown) {
            // Right drag: pan model in screen space (scale sensitivity with zoom)
            float sens = 0.001f * g_camera.camera_distance;
            float rx = g_camera.camera_rotx * 3.14159f / 180.0f;
            float ry = g_camera.camera_roty * 3.14159f / 180.0f;

            // Camera right & up vectors in world space
            float rX =  cosf(ry);
            float rZ = -sinf(ry);
            float uX = -sinf(ry) * sinf(rx);
            float uY =  cosf(rx);
            float uZ = -cosf(ry) * sinf(rx);

            // Screen delta: drag right → model moves right on screen
            float sdx = -dx * sens;
            float sdy =  dy * sens;

            g_model.model_x += sdx * rX + sdy * uX;
            g_model.model_y += sdy * uY;
            g_model.model_z += sdx * rZ + sdy * uZ;
            InvalidateRect(hWnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_MOUSEWHEEL: {
        int delta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
        g_camera.camera_distance -= delta * 0.3f;
        if (g_camera.camera_distance < 0.5f)  g_camera.camera_distance = 0.5f;
        if (g_camera.camera_distance > 50.0f) g_camera.camera_distance = 50.0f;
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1; // prevent flicker

    case WM_DESTROY:
        if (g_hGLRC) {
            wglMakeCurrent(NULL, NULL);
            wglDeleteContext(g_hGLRC);
            g_hGLRC = NULL;
        }
        if (g_hGLDC) {
            ReleaseDC(hWnd, g_hGLDC);
            g_hGLDC = NULL;
        }
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// ============================================================
// Main Window Proc
// ============================================================
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        g_hMainWnd = hWnd;

        // Create the OpenGL child window
        WNDCLASS wcGL = {};
        wcGL.lpfnWndProc = OpenGLWndProc;
        wcGL.hInstance = GetModuleHandle(NULL);
        wcGL.hCursor = LoadCursor(NULL, IDC_ARROW);
        wcGL.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wcGL.lpszClassName = L"OpenGLWindow";
        RegisterClass(&wcGL);

        g_hGLWnd = CreateWindow(L"OpenGLWindow", NULL,
            WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
            0, 0, 100, 100, hWnd, NULL, GetModuleHandle(NULL), NULL);

        // Init GDI+
        GdiplusStartup(&g_gdiToken, &g_gdiStartupInput, NULL);

        // Init buttons
        InitPanelButtons();
        return 0;
    }

    case WM_SIZE: {
        int w = LOWORD(lParam);
        int h = HIWORD(lParam);
        int glw = w - PANEL_WIDTH;
        if (glw < 100) glw = 100;
        g_glWidth = glw;
        g_glHeight = h;
        MoveWindow(g_hGLWnd, 0, 0, glw, h, TRUE);

        // Invalidate the panel area
        RECT r;
        r.left = glw;
        r.top = 0;
        r.right = w;
        r.bottom = h;
        InvalidateRect(hWnd, &r, FALSE);
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT clientRect;
        GetClientRect(hWnd, &clientRect);

        // Draw the right panel with GDI+ (single backbuffer blit, no flicker)
        RECT panelRect;
        panelRect.left = clientRect.right - PANEL_WIDTH;
        panelRect.top = 0;
        panelRect.right = clientRect.right;
        panelRect.bottom = clientRect.bottom;
        DrawPanel(hdc, panelRect);

        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_MOUSEMOVE: {
        int mx = LOWORD(lParam);
        int my = HIWORD(lParam);
        int glw = g_glWidth;

        // Only handle mouse in panel area
        if (mx < glw) break;

        int relX = mx - glw;
        int relY = my;
        int btnIdx = HitTestButtons(relX, relY);
        int prevHover = g_hoverBtn;
        g_hoverBtn = btnIdx;

        // Update hover state
        if (prevHover != btnIdx) {
            if (prevHover >= 0) {
                g_buttons[prevHover].hover = false;
                g_buttons[prevHover].hoverSub = 0;
            }
            if (btnIdx >= 0) {
                g_buttons[btnIdx].hover = true;
                g_buttons[btnIdx].hoverSub = HitTestDirSub(g_buttons[btnIdx], relX, relY);
            }

            RECT client;
            GetClientRect(hWnd, &client);
            RECT r;
            r.left = glw; r.top = 0; r.right = glw + PANEL_WIDTH;
            r.bottom = client.bottom;
            InvalidateRect(hWnd, &r, FALSE);
        } else if (btnIdx >= 0 && g_buttons[btnIdx].type == BTN_DIRECTION) {
            int sub = HitTestDirSub(g_buttons[btnIdx], relX, relY);
            if (g_buttons[btnIdx].hoverSub != sub) {
                g_buttons[btnIdx].hoverSub = sub;
                RECT client2;
                GetClientRect(hWnd, &client2);
                RECT r2;
                r2.left = glw; r2.top = 0; r2.right = glw + PANEL_WIDTH;
                r2.bottom = client2.bottom;
                InvalidateRect(hWnd, &r2, FALSE);
            }
        }
        break;
    }

    case WM_LBUTTONDOWN: {
        int mx = LOWORD(lParam);
        int my = HIWORD(lParam);
        int glw = g_glWidth;

        if (mx < glw) break;

        int relX = mx - glw;
        int relY = my;
        int btnIdx = HitTestButtons(relX, relY);

        if (btnIdx >= 0) {
            GButton& b = g_buttons[btnIdx];
            b.pressed = true;

            RECT client3;
            GetClientRect(hWnd, &client3);
            RECT r3;
            r3.left = glw; r3.top = 0; r3.right = glw + PANEL_WIDTH;
            r3.bottom = client3.bottom;
            InvalidateRect(hWnd, &r3, FALSE);

            // Execute action
            int sub = 0;
            if (b.type == BTN_DIRECTION) {
                sub = HitTestDirSub(b, relX, relY);
            }
            if (b.type == BTN_TOGGLE) {
                sub = 1; // toggle
            }

            ExecuteAction(b.actionId, sub);

            // Invalidate OpenGL window
            InvalidateRect(g_hGLWnd, NULL, FALSE);

            // Reset pressed state after short delay
            SetTimer(hWnd, 100 + btnIdx, 120, NULL);
        }
        break;
    }

    case WM_TIMER: {
        int id = (int)wParam - 100;
        if (id >= 0 && id < g_btnCount) {
            g_buttons[id].pressed = false;
            RECT client4;
            GetClientRect(hWnd, &client4);
            RECT r4;
            r4.left = g_glWidth; r4.top = 0; r4.right = g_glWidth + PANEL_WIDTH;
            r4.bottom = client4.bottom;
            InvalidateRect(hWnd, &r4, FALSE);
            KillTimer(hWnd, wParam);
        }
        break;
    }

    case WM_MOUSEWHEEL: {
        int delta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
        g_panelScrollOffset -= delta * 20;
        if (g_panelScrollOffset < 0) g_panelScrollOffset = 0;
        int maxScroll = g_panelContentHeight - g_glHeight + 60;
        if (g_panelScrollOffset > maxScroll) g_panelScrollOffset = maxScroll;
        if (maxScroll < 0) g_panelScrollOffset = 0;

        RECT client5;
        GetClientRect(hWnd, &client5);
        RECT r5;
        r5.left = g_glWidth; r5.top = 0; r5.right = g_glWidth + PANEL_WIDTH;
        r5.bottom = client5.bottom;
        InvalidateRect(hWnd, &r5, FALSE);
        break;
    }

    case WM_KEYDOWN: {
        switch (wParam) {
        case VK_SPACE: ActionToggleWireframe(); break;
        case 'V':      if (g_isOrtho) { ActionToggleProjection(); } break;
        case 'v':      if (!g_isOrtho) { ActionToggleProjection(); } break;
        case 'O': case 'o': ActionOpenModel(); break;
        case 'S': case 's': ActionResetAll(); break;
        case 'Q': case 'q': ActionQuit(); break;
        }
        // Update panel (refresh toggle states)
        RECT r;
        GetClientRect(hWnd, &r);
        r.left = g_glWidth;
        InvalidateRect(hWnd, &r, FALSE);
        break;
    }

    case WM_DESTROY:
        if (g_displayList != 0) {
            if (g_hGLDC && g_hGLRC) {
                wglMakeCurrent(g_hGLDC, g_hGLRC);
                glDeleteLists(g_displayList, 1);
                wglMakeCurrent(NULL, NULL);
            }
        }
        GdiplusShutdown(g_gdiToken);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// ============================================================
// WinMain - Entry Point
// ============================================================
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow) {

    // Parse command line for OBJ file
    char filename[MAX_PATH] = "";
    if (lpCmdLine && strlen(lpCmdLine) > 0) {
        strcpy_s(filename, MAX_PATH, lpCmdLine);
        // Remove surrounding quotes if any
        int len = (int)strlen(filename);
        if (len > 1 && filename[0] == '"' && filename[len - 1] == '"') {
            filename[len - 1] = '\0';
            memmove(filename, filename + 1, len - 1);
        }
    }

    // Pre-load the model data before OpenGL init
    if (filename[0] != '\0') {
        // We'll load after window creation since we need OpenGL context for display list
        // But we can parse the file now
        LoadOBJFile(filename);
    }

    // Register main window class
    WNDCLASS wc = {};
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = L"ModelViewerMain";
    wc.style = CS_HREDRAW | CS_VREDRAW;

    if (!RegisterClass(&wc)) {
        MessageBox(NULL, L"窗口注册失败", L"错误", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Create main window
    int winW = 1200;
    int winH = 750;
    HWND hWnd = CreateWindowEx(
        0,
        L"ModelViewerMain", L"3D Model Viewer - OpenGL + GDI+",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, winW, winH,
        NULL, NULL, hInstance, NULL);

    if (!hWnd) {
        MessageBox(NULL, L"窗口创建失败", L"错误", MB_OK | MB_ICONERROR);
        return 1;
    }

    ShowWindow(hWnd, nCmdShow);
    // Force immediate repaint so model is visible from the start
    if (g_hGLWnd) {
        InvalidateRect(g_hGLWnd, NULL, FALSE);
        UpdateWindow(g_hGLWnd);
    }
    UpdateWindow(hWnd);

    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
