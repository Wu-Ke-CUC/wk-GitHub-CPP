// Vehicle.h - Vehicle
#pragma once

#include "Math3D.h"

class InputManager;

class Vehicle
{
public:
    Vehicle();
    ~Vehicle();

    void Initialize();
    void Update(InputManager* input, float deltaTime);
    void Render();

    Vec3 GetPosition() const { return m_transform.position; }
    void SetPosition(const Vec3& pos) { m_transform.position = pos; }

    float GetRotationY() const { return m_transform.rotation.y; }
    void SetRotationY(float angle) { m_transform.rotation.y = angle; }

    float GetSpeed() const { return m_speed; }

    Transform& GetTransform() { return m_transform; }

private:
    void UpdatePosition(InputManager* input, float deltaTime);
    void UpdateRotation(InputManager* input, float deltaTime);

    Transform m_transform;
    float m_speed;
    float m_maxSpeed;
    float m_acceleration;
    float m_turnSpeed;
    float m_friction;

    Vec3 m_bodyOffset;
    Vec3 m_topOffset;
    std::vector<Vec3> m_wheelOffsets;
    std::vector<Vec3> m_headlightOffsets;
};
