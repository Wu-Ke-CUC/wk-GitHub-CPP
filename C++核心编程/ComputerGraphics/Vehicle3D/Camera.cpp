// Camera.cpp - Camera Implementation
#include "Include/stdafx.h"
#include "Include/Camera.h"
#include "Include/Vehicle.h"
#include "Include/InputManager.h"

extern Vehicle* g_vehicle;

Camera::Camera()
    : m_fov(60.0f)
    , m_aspectRatio(16.0f / 9.0f)
    , m_nearPlane(0.1f)
    , m_farPlane(1000.0f)
    , m_cameraDistance(15.0f)
    , m_cameraHeight(5.0f)
    , m_cameraAngleX(0)
    , m_cameraAngleY(0)
{
}

Camera::~Camera()
{
}

void Camera::Initialize()
{
    ResetCamera();
}

void Camera::Update(Target* target, InputManager* input, float deltaTime)
{
    if(target) UpdateFollowMode(target, input, deltaTime);
}

void Camera::UpdateFollowMode(Target* target, InputManager* input, float deltaTime)
{
    float cameraSpeed = 0.02f * deltaTime * 60.0f;


    //TODO
    // W/S键控制相机偏移距离 m_cameraDistance
    if (input->IsKeyPressed('W'))
        m_cameraDistance -= cameraSpeed * 10;
    if (input->IsKeyPressed('S'))
        m_cameraDistance += cameraSpeed * 10;
    if (m_cameraDistance < 1)m_cameraDistance = 1;
    // A/D键控制相机水平角度 m_cameraAngleY
    if (input->IsKeyPressed('A'))
        m_cameraAngleY += cameraSpeed;
    if (input->IsKeyPressed('D'))
        m_cameraAngleY -= cameraSpeed;
    // E/Q键控制相机垂直高度 m_cameraHeight
    if (input->IsKeyPressed('E'))
        m_cameraHeight += cameraSpeed * 10;
    if (input->IsKeyPressed('Q'))
        m_cameraHeight -= cameraSpeed * 10;
}

void Camera::ApplyProjection()
{
    gluPerspective(m_fov, m_aspectRatio, m_nearPlane, m_farPlane);
}

void Camera::ApplyView(Target* target)
{
    if (target)
    {
        Vec3 targetPos = target->GetPosition();
        float targetRotation = target->GetRotationY();

        //TODO
        // 使用 m_cameraAngleY 调整相机水平旋转
        targetRotation -= m_cameraAngleY;

        float dirX = cosf(targetRotation);
        float dirZ = -sinf(targetRotation);

        float camX = targetPos.x - dirX * m_cameraDistance;
        float camZ = targetPos.z - dirZ * m_cameraDistance;
        float camY = targetPos.y + m_cameraHeight;

        float lookX = targetPos.x ;//sinf(vehicleRotation) * 5.0f
        float lookZ = targetPos.z ;//cosf(vehicleRotation) * 5.0f
        float lookY = targetPos.y + 1.0f;

        gluLookAt(camX, camY, camZ, lookX, lookY, lookZ, 0, 1, 0);
    }
}

void Camera::ResetCamera()
{
    m_cameraAngleX = 0;
    m_cameraAngleY = 0;
    m_cameraDistance = 15;
    m_cameraHeight = 5;
}
