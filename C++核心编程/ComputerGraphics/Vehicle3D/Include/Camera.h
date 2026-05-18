// Camera.h - Camera
#pragma once

#include "Math3D.h"

class Vehicle;
class InputManager;

class Camera
{
public:
    Camera();
    ~Camera();

    void Initialize();
    void Update(Vehicle* vehicle, InputManager* input, float deltaTime);
    void ApplyProjection();
    void ApplyView();

    void SetAspectRatio(float ratio) { m_aspectRatio = ratio; }
    float GetAspectRatio() const { return m_aspectRatio; }

    float GetFOV() const { return m_fov; }
    void SetFOV(float fov) { m_fov = fov; }

    float GetNearPlane() const { return m_nearPlane; }
    float GetFarPlane() const { return m_farPlane; }

    float GetCameraAngleX() const { return m_cameraAngleX; }
    float GetCameraAngleY() const { return m_cameraAngleY; }

    void ResetCamera();

private:
    void UpdateFollowMode(Vehicle* vehicle, InputManager* input, float deltaTime);

    float m_fov;
    float m_aspectRatio;
    float m_nearPlane;
    float m_farPlane;

    float m_cameraDistance;
    float m_cameraHeight;
    float m_cameraAngleX;
    float m_cameraAngleY;
};
