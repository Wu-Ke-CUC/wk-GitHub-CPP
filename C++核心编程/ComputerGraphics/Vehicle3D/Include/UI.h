// UI.h - User Interface
#pragma once

#include <string>
#include <vector>
#include <windows.h>

class Renderer;
class Target;
class Camera;

struct UIButton
{
    std::wstring text;
    int x, y, width, height;
    bool isHovered;
    bool isActive;
    void (*onClick)();
};

struct UIText
{
    std::wstring text;
    int x, y;
    bool isTitle;
};

class UI
{
public:
    UI(HWND hWnd);
    ~UI();

    void Update(Target* target, Camera* camera);
    void Render(Renderer* renderer);

    void OnMouseDown(int x, int y);
    void OnMouseUp();
    void OnMouseMove(int x, int y);
    bool IsDragging() const { return m_isDragging; }

    void Resize(int width, int height);

    HWND GetHWnd() const { return m_hWnd; }

private:
    void CreateFontDisplayLists();
    void RenderBackground(int x, int y, int width, int height, float alpha = 0.7f);
    void RenderTextOpenGL(const wchar_t* text, int x, int y, bool title);
    void DrawButtonOpenGL(const wchar_t* text, int x, int y, bool title, bool isActive);
    bool IsPointInRect(int px, int py, int x, int y, int w, int h);
    void DrawTextGDI(const wchar_t* text, int x, int y, bool title = false);
    void DrawButton(const UIButton& button);

    HWND m_hWnd;
    int m_width;
    int m_height;

    int m_mouseX;
    int m_mouseY;
    bool m_isDragging;

    std::vector<UIButton> m_buttons;

    std::wstring m_vehiclePosition;
    std::wstring m_vehicleSpeed;
    std::wstring m_cameraAngle;

    int m_uiX;
    int m_uiY;
    int m_uiWidth;
    int m_uiHeight;

    int m_statusX;
    int m_statusY;
    int m_statusWidth;
    int m_statusHeight;

    // Font display lists
    GLuint m_fontListBase;
    GLuint m_normalFontBase;
};
