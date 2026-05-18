#pragma once

#include "Math3D.h"

class InputManager;

// Target.h - 可跟随目标基类
class Target
{
public:
    virtual ~Target() {}

    virtual void Initialize() = 0;
    virtual void Update(InputManager* input, float deltaTime) = 0;
    virtual void Render() = 0;

    virtual Vec3 GetPosition() const = 0;
    virtual float GetRotationY() const = 0;
    virtual float GetSpeed() const = 0;

    virtual const wchar_t* GetTypeName() const = 0;  // 用于UI显示
};

// Vehicle.h - Vehicle
class Vehicle : public Target
{
public:
    Vehicle();
    ~Vehicle();

    void Initialize() override;
    void Update(InputManager* input, float deltaTime) override;
    void Render() override;

    Vec3 GetPosition() const override { return m_transform.position; }
    void SetPosition(const Vec3& pos) { m_transform.position = pos; }

    float GetRotationY() const override { return m_transform.rotation.y; }
    void SetRotationY(float angle) { m_transform.rotation.y = angle; }

    float GetSpeed() const override { return m_speed; }

    const wchar_t* GetTypeName() const override { return L"Vehicle"; }

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

// Aircraft.h - Aircraft
class Aircraft : public Target
{
public:
    Aircraft();
    ~Aircraft();

    void Initialize() override;
    void Update(InputManager* input, float deltaTime) override;
    void Render() override;

    Vec3 GetPosition() const override { return m_position; }
    float GetRotationY() const override { return m_rotationY; }
    float GetSpeed() const override { return m_speed; }
    const wchar_t* GetTypeName() const override { return L"Aircraft"; }

private:
    Vec3 m_position;
    float m_rotationY;
    float m_speed;
    float m_verticalSpeed;   // 升降速度

    // 参数
    float m_maxSpeed;
    float m_acceleration;
    float m_turnSpeed;
    float m_friction;
    float m_maxHeight;
    float m_minHeight;
    float m_verticalAcceleration;
};