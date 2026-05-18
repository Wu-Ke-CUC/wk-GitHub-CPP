// InputManager.cpp - Input Manager Implementation
#include "Include/stdafx.h"
#include "Include/InputManager.h"

InputManager::InputManager()
    : m_hWnd(nullptr)
{
}

InputManager::~InputManager()
{
}

void InputManager::Initialize(HWND hWnd)
{
    m_hWnd = hWnd;
}

void InputManager::KeyDown(int key)
{
    if (!m_keyStates[key])
    {
        m_keyPressedThisFrame[key] = true;
    }
    m_keyStates[key] = true;
}

void InputManager::KeyUp(int key)
{
    m_keyStates[key] = false;
}

bool InputManager::IsKeyPressed(int key)
{
    return m_keyStates[key];
}

void InputManager::Update()
{
    m_keyPressedThisFrame.clear();
}

void InputManager::ReleaseAllKeys()
{
    for (auto& pair : m_keyStates)
    {
        pair.second = false;
    }
}
