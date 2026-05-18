// Aircraft.cpp - 飞行器实现
#include "Include/stdafx.h"
#include "Include/Vehicle.h"
#include "Include/InputManager.h"
#include "Include/Renderer.h"

extern Renderer* g_renderer;

Aircraft::Aircraft()
    : m_position(0, 5, 0)      // 初始悬空
    , m_rotationY(0)
    , m_speed(0)
    , m_verticalSpeed(0)
    , m_maxSpeed(4.0f)
    , m_acceleration(0.08f)
    , m_turnSpeed(0.04f)
    , m_friction(0.98f)
    , m_maxHeight(30.0f)
    , m_minHeight(2.0f)
    , m_verticalAcceleration(0.15f)
{}

Aircraft::~Aircraft()
{}

void Aircraft::Initialize()
{
    m_position.Set(0, 8, 0);
    m_rotationY = -PI / 2;
    m_speed = 0;
    m_verticalSpeed = 0;
}

void Aircraft::Update(InputManager* input, float deltaTime)
{
    // 加速 / 减速 (方向键上下)
    float forward = 0;
    if (input->IsKeyPressed(VK_UP))
        forward = 1;
    else if (input->IsKeyPressed(VK_DOWN))
        forward = -1;

    if (forward != 0)
    {
        m_speed += forward * m_acceleration * deltaTime * 60.0f;
        m_speed = Math::Clamp(m_speed, -m_maxSpeed * 0.5f, m_maxSpeed);
    }
    else
    {
        m_speed *= m_friction;
        if (fabsf(m_speed) < 0.01f) m_speed = 0;
    }

    // 转向 (方向键左右)
    if (input->IsKeyPressed(VK_LEFT))
        m_rotationY += m_turnSpeed * deltaTime * 60.0f;
    if (input->IsKeyPressed(VK_RIGHT))
        m_rotationY -= m_turnSpeed * deltaTime * 60.0f;

    // 升降 (R / F)
    if (input->IsKeyPressed(VK_SPACE))
        m_verticalSpeed += m_verticalAcceleration * deltaTime * 60.0f;
    if (input->IsKeyPressed(VK_SHIFT))
        m_verticalSpeed -= m_verticalAcceleration * deltaTime * 60.0f;
    m_verticalSpeed *= 0.98f;
    m_position.y += m_verticalSpeed * deltaTime * 60.0f;
    m_position.y = Math::Clamp(m_position.y, m_minHeight, m_maxHeight);

    // 移动
    float dirX = cosf(m_rotationY);
    float dirZ = -sinf(m_rotationY);
    m_position.x += dirX * m_speed * deltaTime * 60.0f;
    m_position.z += dirZ * m_speed * deltaTime * 60.0f;

    // 边界限制 (同车辆范围)
    float bound = 90.0f;
    m_position.x = Math::Clamp(m_position.x, -bound, bound);
    m_position.z = Math::Clamp(m_position.z, -bound, bound);
}

void Aircraft::Render()
{
    glPushMatrix();

    glTranslatef(m_position.x, m_position.y, m_position.z);
    glRotatef(RAD_TO_DEG(m_rotationY), 0, 1, 0);

    // 机身 (长方体)
    glColor4f(0.2f, 0.6f, 0.9f, 1.0f);
    g_renderer->DrawBox(Vec3(0, 0, 0), 2.0f, 0.2f, 3.0f);

    // 机翼 (扁长方体)
    glColor4f(0.3f, 0.7f, 1.0f, 1.0f);
    g_renderer->DrawBox(Vec3(0, 0.1f, 0), 4.0f, 0.5f, 1.2f);

    // 尾翼
    glColor4f(0.2f, 0.5f, 0.8f, 1.0f);
    g_renderer->DrawBox(Vec3(-1.2, 0.3f, 0.0f), 0.8f, 0.8f, 0.4f);

    // 螺旋桨 (小圆柱)
    glColor4f(0.5f, 0.5f, 0.5f, 1.0f);
    glPushMatrix();
    glTranslatef(1.8f, 0.2f, 0.0f);
    glRotatef(90, 0, 1, 0);
    g_renderer->DrawCylinder(Vec3(0, 0, 0), 0.2f, 1.2f, 8);
    glPopMatrix();

    glPopMatrix();
}