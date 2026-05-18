// UI.cpp - User Interface Implementation
#include "Include/stdafx.h"
#include "Include/UI.h"
#include "Include/Renderer.h"
#include "Include/Vehicle.h"
#include "Include/Camera.h"

UI::UI(HWND hWnd)
    : m_hWnd(hWnd)
    , m_width(1920)
    , m_height(1080)
    , m_mouseX(0)
    , m_mouseY(0)
    , m_isDragging(false)
    , m_uiX(20)
    , m_uiY(20)
    , m_uiWidth(300)
    , m_uiHeight(230)
    , m_statusX(0)
    , m_statusY(20)
    , m_statusWidth(250)
    , m_statusHeight(130)
    , m_fontListBase(0)
{
    Resize(m_width, m_height);
    CreateFontDisplayLists();
}

UI::~UI()
{
    if (m_fontListBase != 0)
    {
        glDeleteLists(m_fontListBase, 256);
    }
}

void UI::CreateFontDisplayLists()
{
    HDC hDC = wglGetCurrentDC();
    
    // Title font - bold, larger
    HFONT hTitleFont = CreateFont(
        24, 0, 0, 0,
        FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
    );
    
    // Normal font, larger
    HFONT hNormalFont = CreateFont(
        18, 0, 0, 0,
        FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
    );
    
    // Create display lists for title font (indices 0-127)
    HFONT oldFont = (HFONT)SelectObject(hDC, hTitleFont);
    m_fontListBase = glGenLists(256);
    wglUseFontBitmaps(hDC, 0, 128, m_fontListBase);
    
    // Create display lists for normal font (indices 256-383)
    SelectObject(hDC, hNormalFont);
    m_normalFontBase = m_fontListBase + 256;
    wglUseFontBitmaps(hDC, 0, 128, m_normalFontBase);
    
    SelectObject(hDC, oldFont);
    DeleteObject(hTitleFont);
    DeleteObject(hNormalFont);
}

void UI::Resize(int width, int height)
{
    m_width = width;
    m_height = height;
    // Left panel: top-left corner
    m_uiX = 20;
    m_uiY = 20;  // top position
    // Right panel: bottom-right corner, leave one-line gap at bottom (38 = 20 + 18)
    m_statusX = width - m_statusWidth - 20;
    m_statusY = height - m_statusHeight - 38;  // bottom position with one-line gap
}

void UI::Update(Vehicle* vehicle, Camera* camera)
{
    Vec3 pos = vehicle->GetPosition();
    wchar_t buf[128];

    swprintf(buf, 128, L"%.1f, %.1f, %.1f", pos.x, pos.y, pos.z);
    m_vehiclePosition = buf;

    swprintf(buf, 128, L"%.1f", vehicle->GetSpeed());
    m_vehicleSpeed = buf;

    float angleX = RAD_TO_DEG(camera->GetCameraAngleX());
    float angleY = RAD_TO_DEG(camera->GetCameraAngleY());
    swprintf(buf, 128, L"%.0f deg, %.0f deg", angleX, angleY);
    m_cameraAngle = buf;
}

void UI::Render(Renderer* renderer)
{
    // Switch to 2D orthographic projection
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, m_width, m_height, 0, -1, 1);  // Y flipped: top=0, bottom=height

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Render left panel (top-left)
    RenderBackground(m_uiX, m_uiY, m_uiWidth, m_uiHeight, 0.5f);
    RenderTextOpenGL(L"3D Vehicle Camera Control", m_uiX + 15, m_uiY + 2, true);

    glColor4f(0.2f, 0.55f, 0.94f, 1.0f);
    glBegin(GL_LINES);
    glVertex2i(m_uiX + 10, m_uiY + 32);
    glVertex2i(m_uiX + m_uiWidth - 10, m_uiY + 32);
    glEnd();

    RenderTextOpenGL(L"Vehicle Control (Arrow Keys)", m_uiX + 15, m_uiY + 52, false);
    RenderTextOpenGL(L"UP: Forward  DOWN: Back", m_uiX + 15, m_uiY + 77, false);
    RenderTextOpenGL(L"LEFT: Turn Left  RIGHT: Turn Right", m_uiX + 15, m_uiY + 97, false);
    RenderTextOpenGL(L"Camera Control (WSAD)", m_uiX + 15, m_uiY + 122, false);
    RenderTextOpenGL(L"W: Up  S: Down  A: Left  D: Right", m_uiX + 15, m_uiY + 147, false);

    // Buttons
    int btnY = m_uiY + 177;
    int btnWidth = 115;
    int btnHeight = 28;
    
    // Reset Camera button
    RenderBackground(m_uiX + 25, btnY, btnWidth, btnHeight, 0.7f);
    RenderTextOpenGL(L"Reset [F2]", m_uiX + 35, btnY + 6, false);

    // Check for button clicks
    extern HWND g_hWnd;
    extern void ResetCameraCallback();
    POINT pt;
    GetCursorPos(&pt);
    ScreenToClient(g_hWnd, &pt);
    
    // Reset button click area
    if (pt.x >= m_uiX + 25 && pt.x <= m_uiX + 25 + btnWidth &&
        pt.y >= btnY && pt.y <= btnY + btnHeight)
    {
        if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
            ResetCameraCallback();
    }

    // Render right panel (bottom-right)
    RenderBackground(m_statusX, m_statusY, m_statusWidth, m_statusHeight, 0.5f);
    RenderTextOpenGL(L"Status", m_statusX + 15, m_statusY + 2, true);

    glColor4f(0.2f, 0.55f, 0.94f, 1.0f);
    glBegin(GL_LINES);
    glVertex2i(m_statusX + 10, m_statusY + 32);
    glVertex2i(m_statusX + m_statusWidth - 10, m_statusY + 32);
    glEnd();

    int infoY = m_statusY + 52;
    RenderTextOpenGL(L"Position:", m_statusX + 15, infoY, false);
    RenderTextOpenGL(m_vehiclePosition.c_str(), m_statusX + 95, infoY, false);
    RenderTextOpenGL(L"Speed:", m_statusX + 15, infoY + 25, false);
    RenderTextOpenGL(m_vehicleSpeed.c_str(), m_statusX + 95, infoY + 25, false);
    RenderTextOpenGL(L"Angle:", m_statusX + 15, infoY + 50, false);
    RenderTextOpenGL(m_cameraAngle.c_str(), m_statusX + 95, infoY + 50, false);

    // Restore 3D projection
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void UI::RenderTextOpenGL(const wchar_t* text, int x, int y, bool title)
{
    if (m_fontListBase == 0 || text == nullptr) return;

    int len = 0;
    while (text[len] != 0) len++;
    if (len == 0) return;

    // Convert wchar_t to byte array for glCallLists
    std::vector<GLubyte> byteText;
    byteText.reserve(len);
    for (int i = 0; i < len; i++)
    {
        byteText.push_back(static_cast<GLubyte>(text[i] & 0xFF));
    }

    glColor4f(title ? 0.302f : 0.671f, title ? 0.671f : 0.902f, title ? 0.969f : 0.902f, 1.0f);
    glRasterPos2i(x, y + (title ? 24 : 18));
    
    glListBase(title ? m_fontListBase : m_normalFontBase);
    glCallLists((GLsizei)len, GL_UNSIGNED_BYTE, byteText.data());
}

void UI::DrawButtonOpenGL(const wchar_t* text, int x, int y, bool title, bool isActive)
{
    // Button background is drawn in Render() via DrawButton, just draw text here
    RenderTextOpenGL(text, x, y, title);
}

void UI::RenderBackground(int x, int y, int width, int height, float alpha)
{
    glColor4f(0.0f, 0.0f, 0.0f, alpha);
    glBegin(GL_QUADS);
    glVertex2i(x, y);
    glVertex2i(x + width, y);
    glVertex2i(x + width, y + height);
    glVertex2i(x, y + height);
    glEnd();

    glColor4f(0.2f, 0.2f, 0.2f, alpha);
    glLineWidth(1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2i(x, y);
    glVertex2i(x + width, y);
    glVertex2i(x + width, y + height);
    glVertex2i(x, y + height);
    glEnd();
}

bool UI::IsPointInRect(int px, int py, int x, int y, int w, int h)
{
    return (px >= x && px <= x + w && py >= y && py <= y + h);
}

void UI::DrawTextGDI(const wchar_t* text, int x, int y, bool title)
{
    // Deprecated - kept for compatibility
}

void UI::DrawButton(const UIButton& button)
{
    RenderBackground(button.x, button.y, button.width, button.height, 0.8f);

    if (button.isActive)
    {
        glColor4f(0.094f, 0.392f, 0.718f, 1.0f);
        glBegin(GL_QUADS);
        glVertex2i(button.x + 1, button.y + 1);
        glVertex2i(button.x + button.width - 1, button.y + 1);
        glVertex2i(button.x + button.width - 1, button.y + button.height - 1);
        glVertex2i(button.x + 1, button.y + button.height - 1);
        glEnd();
    }
}

void UI::OnMouseDown(int x, int y)
{
    m_mouseX = x;
    m_mouseY = y;
    m_isDragging = true;
}

void UI::OnMouseUp()
{
    m_isDragging = false;
}

void UI::OnMouseMove(int x, int y)
{
    m_mouseX = x;
    m_mouseY = y;
}
