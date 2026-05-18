// Math3D.h - 3D Math Library
#pragma once

#ifndef PI
#define PI 3.14159265358979323846f
#endif

#define DEG_TO_RAD(x) ((x) * PI / 180.0f)
#define RAD_TO_DEG(x) ((x) * 180.0f / PI)

// Vector class
class Vec3
{
public:
    float x, y, z;

    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    Vec3 operator+(const Vec3& v) const { return Vec3(x + v.x, y + v.y, z + v.z); }
    Vec3 operator-(const Vec3& v) const { return Vec3(x - v.x, y - v.y, z - v.z); }
    Vec3 operator*(float scalar) const { return Vec3(x * scalar, y * scalar, z * scalar); }
    Vec3 operator/(float scalar) const { return Vec3(x / scalar, y / scalar, z / scalar); }

    Vec3& operator+=(const Vec3& v) { x += v.x; y += v.y; z += v.z; return *this; }
    Vec3& operator-=(const Vec3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
    Vec3& operator*=(float scalar) { x *= scalar; y *= scalar; z *= scalar; return *this; }

    Vec3 operator-() const { return Vec3(-x, -y, -z); }

    float Length() const { return sqrtf(x * x + y * y + z * z); }
    float LengthSquared() const { return x * x + y * y + z * z; }

    Vec3 Normalized() const
    {
        float len = Length();
        if (len > 0.0001f)
            return Vec3(x / len, y / len, z / len);
        return Vec3(0, 0, 0);
    }

    void Normalize()
    {
        float len = Length();
        if (len > 0.0001f)
        {
            x /= len; y /= len; z /= len;
        }
    }

    float Dot(const Vec3& v) const { return x * v.x + y * v.y + z * v.z; }

    Vec3 Cross(const Vec3& v) const
    {
        return Vec3(
            y * v.z - z * v.y,
            z * v.x - x * v.z,
            x * v.y - y * v.x
        );
    }

    Vec3 Lerp(const Vec3& v, float t) const
    {
        return Vec3(
            x + (v.x - x) * t,
            y + (v.y - y) * t,
            z + (v.z - z) * t
        );
    }

    static Vec3 Zero() { return Vec3(0, 0, 0); }
    static Vec3 Up() { return Vec3(0, 1, 0); }
    static Vec3 Right() { return Vec3(1, 0, 0); }
    static Vec3 Forward() { return Vec3(0, 0, 1); }

    void Set(float nx, float ny, float nz) { x = nx; y = ny; z = nz; }

    void RotateY(float angle)
    {
        float cosA = cosf(angle);
        float sinA = sinf(angle);
        float nx = x * cosA + z * sinA;
        float nz = -x * sinA + z * cosA;
        x = nx;
        z = nz;
    }
};

// 2D Vector
class Vec2
{
public:
    float x, y;

    Vec2() : x(0), y(0) {}
    Vec2(float x, float y) : x(x), y(y) {}

    Vec2 operator+(const Vec2& v) const { return Vec2(x + v.x, y + v.y); }
    Vec2 operator-(const Vec2& v) const { return Vec2(x - v.x, y - v.y); }
    Vec2 operator*(float scalar) const { return Vec2(x * scalar, y * scalar); }

    float Length() const { return sqrtf(x * x + y * y); }
    void Normalize()
    {
        float len = Length();
        if (len > 0.0001f)
        {
            x /= len; y /= len;
        }
    }
};

// Color class
class Color
{
public:
    float r, g, b, a;

    Color() : r(1), g(1), b(1), a(1) {}
    Color(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}
    Color(unsigned int hex)
    {
        r = ((hex >> 16) & 0xFF) / 255.0f;
        g = ((hex >> 8) & 0xFF) / 255.0f;
        b = (hex & 0xFF) / 255.0f;
        a = 1.0f;
    }

    static Color White() { return Color(1, 1, 1, 1); }
    static Color Black() { return Color(0, 0, 0, 1); }
    static Color Red() { return Color(1, 0, 0, 1); }
    static Color Green() { return Color(0, 1, 0, 1); }
    static Color Blue() { return Color(0, 0, 1, 1); }
    static Color Yellow() { return Color(1, 1, 0, 1); }
    static Color Orange() { return Color(1, 0.647f, 0, 1); }
    static Color Gray() { return Color(0.5f, 0.5f, 0.5f, 1); }

    // HTML colors
    static Color SkyBlue() { return Color(0x87CEEB); }
    static Color GrassGreen() { return Color(0x7cfc00); }
    static Color OrangeRed() { return Color(0xff4500); }
    static Color Tomato() { return Color(0xff6347); }
    static Color SaddleBrown() { return Color(0x8B4513); }
    static Color ForestGreen() { return Color(0x228B22); }
};

// Transform class
class Transform
{
public:
    Vec3 position;
    Vec3 rotation;  // radians
    Vec3 scale;

    Transform() : position(0, 0, 0), rotation(0, 0, 0), scale(1, 1, 1) {}

    void Apply()
    {
        glTranslatef(position.x, position.y, position.z);
        glRotatef(RAD_TO_DEG(rotation.z), 0, 0, 1);
        glRotatef(RAD_TO_DEG(rotation.y), 0, 1, 0);
        glRotatef(RAD_TO_DEG(rotation.x), 1, 0, 0);
        glScalef(scale.x, scale.y, scale.z);
    }

    void ApplyNoScale()
    {
        glTranslatef(position.x, position.y, position.z);
        glRotatef(RAD_TO_DEG(rotation.z), 0, 0, 1);
        glRotatef(RAD_TO_DEG(rotation.y), 0, 1, 0);
        glRotatef(RAD_TO_DEG(rotation.x), 1, 0, 0);
    }

    Vec3 GetForward() const
    {
        Vec3 forward(0, 0, 1);
        float cosA = cosf(rotation.y);
        float sinA = sinf(rotation.y);
        forward.x = forward.x * cosA + forward.z * sinA;
        forward.z = -forward.x * sinA + forward.z * cosA;
        return forward;
    }

    Vec3 GetRight() const
    {
        Vec3 right(1, 0, 0);
        float cosA = cosf(rotation.y);
        float sinA = sinf(rotation.y);
        right.x = right.x * cosA + right.z * sinA;
        right.z = -right.x * sinA + right.z * cosA;
        return right;
    }
};

// Bounding Box
class BoundingBox
{
public:
    Vec3 m_min;
    Vec3 m_max;

    BoundingBox() {}
    BoundingBox(const Vec3& vmin, const Vec3& vmax) : m_min(vmin), m_max(vmax) {}

    bool Intersects(const BoundingBox& other) const
    {
        return (m_min.x <= other.m_max.x && m_max.x >= other.m_min.x &&
                m_min.y <= other.m_max.y && m_max.y >= other.m_min.y &&
                m_min.z <= other.m_max.z && m_max.z >= other.m_min.z);
    }

    bool Contains(const Vec3& point) const
    {
        return (point.x >= m_min.x && point.x <= m_max.x &&
                point.y >= m_min.y && point.y <= m_max.y &&
                point.z >= m_min.z && point.z <= m_max.z);
    }

    Vec3 GetCenter() const { return (m_min + m_max) * 0.5f; }
};

// Math utility functions
namespace Math
{
    inline float Clamp(float value, float minVal, float maxVal)
    {
        if (value < minVal) return minVal;
        if (value > maxVal) return maxVal;
        return value;
    }

    inline float Lerp(float a, float b, float t)
    {
        return a + (b - a) * Clamp(t, 0.0f, 1.0f);
    }

    inline float SmoothStep(float a, float b, float t)
    {
        t = Clamp((t - a) / (b - a), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    inline float RandFloat()
    {
        return (float)rand() / (float)RAND_MAX;
    }

    inline float RandRange(float minVal, float maxVal)
    {
        return minVal + RandFloat() * (maxVal - minVal);
    }

    inline int RandInt(int minVal, int maxVal)
    {
        return minVal + rand() % (maxVal - minVal + 1);
    }

    inline bool NearlyEqual(float a, float b, float epsilon = 0.0001f)
    {
        return fabsf(a - b) < epsilon;
    }
}
