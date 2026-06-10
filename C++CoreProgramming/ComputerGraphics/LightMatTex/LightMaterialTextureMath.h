#pragma once

#include "LightMaterialTexture.h"

#include <algorithm>
#include <cmath>

template <typename T>
inline T Clamp(T value, T minValue, T maxValue) {
    return std::max(minValue, std::min(value, maxValue));
}

inline float Saturate(float value) {
    return Clamp(value, 0.0f, 1.0f);
}

inline Vec3 operator+(const Vec3& a, const Vec3& b) {
    return { a.x + b.x, a.y + b.y, a.z + b.z };
}

inline Vec3 operator-(const Vec3& a, const Vec3& b) {
    return { a.x - b.x, a.y - b.y, a.z - b.z };
}

inline Vec3 operator-(const Vec3& v) {
    return { -v.x, -v.y, -v.z };
}

inline Vec3 operator*(const Vec3& v, float s) {
    return { v.x * s, v.y * s, v.z * s };
}

inline Vec3 operator*(float s, const Vec3& v) {
    return v * s;
}

inline Vec3 operator/(const Vec3& v, float s) {
    return { v.x / s, v.y / s, v.z / s };
}

inline Vec3 operator*(const Vec3& a, const Vec3& b) {
    return { a.x * b.x, a.y * b.y, a.z * b.z };
}

inline Vec3& operator+=(Vec3& a, const Vec3& b) {
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
    return a;
}

inline float Dot(const Vec3& a, const Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vec3 Cross(const Vec3& a, const Vec3& b) {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

inline float Length(const Vec3& v) {
    return std::sqrt(Dot(v, v));
}

inline Vec3 Normalize(const Vec3& v) {
    float len = Length(v);
    if (len < 1e-6f) {
        return { 0.0f, 1.0f, 0.0f };
    }
    return v / len;
}

inline Vec3 Abs(const Vec3& v) {
    return { std::fabs(v.x), std::fabs(v.y), std::fabs(v.z) };
}

inline Vec3 Max(const Vec3& v, float value) {
    return { std::max(v.x, value), std::max(v.y, value), std::max(v.z, value) };
}

inline float Max3(const Vec3& v) {
    return std::max(v.x, std::max(v.y, v.z));
}

inline float Fract(float v) {
    return v - std::floor(v);
}

inline Vec3 Lerp(const Vec3& a, const Vec3& b, float t) {
    return a * (1.0f - t) + b * t;
}

inline Vec3 Reflect(const Vec3& incident, const Vec3& normal) {
    return incident - normal * (2.0f * Dot(incident, normal));
}

inline Vec3 RotateX(const Vec3& p, float angle) {
    float c = std::cos(angle);
    float s = std::sin(angle);
    return { p.x, p.y * c - p.z * s, p.y * s + p.z * c };
}

inline Vec3 RotateY(const Vec3& p, float angle) {
    float c = std::cos(angle);
    float s = std::sin(angle);
    return { p.x * c + p.z * s, p.y, -p.x * s + p.z * c };
}
