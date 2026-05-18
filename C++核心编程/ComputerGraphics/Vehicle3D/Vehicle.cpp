// Vehicle.cpp - Vehicle Implementation
#include "Include/stdafx.h"
#include "Include/Vehicle.h"
#include "Include/InputManager.h"
#include "Include/Renderer.h"

extern Renderer* g_renderer;

Vehicle::Vehicle()
    : m_speed(0.0f)
    , m_maxSpeed(2.0f)
    , m_acceleration(0.05f)
    , m_turnSpeed(0.03f)
    , m_friction(0.9f)
{
    m_bodyOffset = Vec3(0, 0, 0);
    m_topOffset = Vec3(0, 1, 0);

    // Wheels: x=±1.3 (front/back), y=0 (cylinder on ground), z=±1.15 (body edge + thickness 0.3)
    m_wheelOffsets.push_back(Vec3(1.3f, 0.0f, 1.15f));   // front-right
    m_wheelOffsets.push_back(Vec3(-1.3f, 0.0f, 1.15f));  // rear-right
    m_wheelOffsets.push_back(Vec3(1.3f, 0.0f, -1.15f));  // front-left
    m_wheelOffsets.push_back(Vec3(-1.3f, 0.0f, -1.15f)); // rear-left

    // Headlights: x=2.0 (front of body), y=0.6 (middle), z=±0.5 (sides)
    m_headlightOffsets.push_back(Vec3(2.0f, 0.6f, 0.5f));
    m_headlightOffsets.push_back(Vec3(2.0f, 0.6f, -0.5f));
}

Vehicle::~Vehicle()
{
}

void Vehicle::Initialize()
{
    m_transform.position.Set(0, 1, 0);
    m_transform.rotation.y = -PI / 2.0f;
}

void Vehicle::Update(InputManager* input, float deltaTime)
{
    UpdatePosition(input, deltaTime);
    UpdateRotation(input, deltaTime);
}

void Vehicle::UpdatePosition(InputManager* input, float deltaTime)
{
    float forward = 0;
    float boundMin = -90.0f;
    float boundMax = 90.0f;
    float bufferZone = 10.0f;  // Buffer zone before boundary

    if (input->IsKeyPressed(VK_UP))
    {
        forward = 1;
    }
    else if (input->IsKeyPressed(VK_DOWN))
    {
        forward = -1;
    }

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

    float dirX = cosf(m_transform.rotation.y);
    float dirZ = -sinf(m_transform.rotation.y);

    // Calculate new position
    float newX = m_transform.position.x + dirX * m_speed;
    float newZ = m_transform.position.z + dirZ * m_speed;

    // Check boundaries and slow down in buffer zone
    bool nearBoundary = false;

    // X boundary check
    if (newX < boundMin + bufferZone)
    {
        // Approaching left boundary
        float ratio = (newX - boundMin) / bufferZone;
        if (ratio < 0) ratio = 0;
        if (dirX < 0) m_speed *= ratio;  // Slow down when moving toward boundary
        if (newX < boundMin) newX = boundMin;  // Hard stop at boundary
        nearBoundary = true;
    }
    else if (newX > boundMax - bufferZone)
    {
        // Approaching right boundary
        float ratio = (boundMax - newX) / bufferZone;
        if (ratio < 0) ratio = 0;
        if (dirX > 0) m_speed *= ratio;  // Slow down when moving toward boundary
        if (newX > boundMax) newX = boundMax;  // Hard stop at boundary
        nearBoundary = true;
    }

    // Z boundary check
    if (newZ < boundMin + bufferZone)
    {
        // Approaching front boundary
        float ratio = (newZ - boundMin) / bufferZone;
        if (ratio < 0) ratio = 0;
        if (dirZ < 0) m_speed *= ratio;  // Slow down when moving toward boundary
        if (newZ < boundMin) newZ = boundMin;  // Hard stop at boundary
        nearBoundary = true;
    }
    else if (newZ > boundMax - bufferZone)
    {
        // Approaching back boundary
        float ratio = (boundMax - newZ) / bufferZone;
        if (ratio < 0) ratio = 0;
        if (dirZ > 0) m_speed *= ratio;  // Slow down when moving toward boundary
        if (newZ > boundMax) newZ = boundMax;  // Hard stop at boundary
        nearBoundary = true;
    }

    // Apply position
    m_transform.position.x = newX;
    m_transform.position.z = newZ;
}

void Vehicle::UpdateRotation(InputManager* input, float deltaTime)
{
    // Allow turning even when stationary
    if (input->IsKeyPressed(VK_LEFT))
    {
        m_transform.rotation.y += m_turnSpeed * deltaTime * 60.0f;
        // If stationary, set minimum speed for turning
        if (fabsf(m_speed) < 0.01f)
            m_speed = 0.1f;
    }
    else if (input->IsKeyPressed(VK_RIGHT))
    {
        m_transform.rotation.y -= m_turnSpeed * deltaTime * 60.0f;
        // If stationary, set minimum speed for turning
        if (fabsf(m_speed) < 0.01f)
            m_speed = 0.1f;
    }
}

void Vehicle::Render()
{
    glPushMatrix();

    glTranslatef(m_transform.position.x, m_transform.position.y, m_transform.position.z);
    glRotatef(RAD_TO_DEG(m_transform.rotation.y), 0, 1, 0);

    // Draw body
    glColor4f(1.0f, 0.27f, 0.0f, 1.0f);
    glPushMatrix();
    glTranslatef(m_bodyOffset.x, m_bodyOffset.y, m_bodyOffset.z);
    g_renderer->DrawBox(Vec3(0, 0, 0), 4.0f, 1.0f, 2.0f);
    glPopMatrix();

    // Draw top
    glColor4f(1.0f, 0.388f, 0.278f, 1.0f);
    glPushMatrix();
    glTranslatef(m_topOffset.x, m_topOffset.y, m_topOffset.z);
    g_renderer->DrawBox(Vec3(0, 0, 0), 2.0f, 1.0f, 1.5f);
    glPopMatrix();

    // Draw wheels
    glColor4f(0.2f, 0.2f, 0.2f, 1.0f);
    for (const auto& offset : m_wheelOffsets)
    {
        glPushMatrix();
        glTranslatef(offset.x, offset.y, offset.z);
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        g_renderer->DrawCylinder(Vec3(0, 0, 0), 0.5f, 0.3f, 16);
        glPopMatrix();
    }

    // Draw headlights
    glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
    glDisable(GL_LIGHTING);
    for (const auto& offset : m_headlightOffsets)
    {
        glPushMatrix();
        glTranslatef(offset.x, offset.y, offset.z);
        g_renderer->DrawSphere(Vec3(0, 0, 0), 0.2f, 8, 8);
        glPopMatrix();
    }
    glEnable(GL_LIGHTING);

    glPopMatrix();
}
