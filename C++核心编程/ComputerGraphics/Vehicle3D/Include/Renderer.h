// Renderer.h - Renderer
#pragma once

class Renderer
{
public:
    Renderer();
    ~Renderer();

    void BeginFrame();
    void EndFrame();

    void SetColor(float r, float g, float b, float a = 1.0f);
    void SetColor(const Color& color);

    void DrawLine(const Vec3& start, const Vec3& end);
    void DrawWireBox(const Vec3& center, float width, float height, float depth);
    void DrawBox(const Vec3& center, float width, float height, float depth);
    void DrawSphere(const Vec3& center, float radius, int segments = 16, int rings = 12);
    void DrawCylinder(const Vec3& center, float radius, float height, int segments = 16);
    void DrawCone(const Vec3& center, float radius, float height, int segments = 16);
    void DrawGrid(float size, float step);

private:
    Color m_currentColor;
};
