// InputManager.h - Input Manager
#pragma once

#include <map>

class InputManager
{
public:
    InputManager();
    ~InputManager();

    void Initialize(HWND hWnd);

    void KeyDown(int key);
    void KeyUp(int key);
    bool IsKeyPressed(int key);

    void Update();
    void ReleaseAllKeys();

private:
    std::map<int, bool> m_keyStates;
    std::map<int, bool> m_keyPressedThisFrame;

    HWND m_hWnd;
};
