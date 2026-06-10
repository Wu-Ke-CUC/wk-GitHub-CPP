// Renderer.cpp - Renderer Implementation
#include "Include/stdafx.h"
#include "Include/Renderer.h"

Renderer::Renderer() : m_currentColor(1, 1, 1, 1)
{
}

Renderer::~Renderer()
{
}

void Renderer::BeginFrame()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::EndFrame()
{
}

void Renderer::SetColor(float r, float g, float b, float a)
{
    m_currentColor.r = r;
    m_currentColor.g = g;
    m_currentColor.b = b;
    m_currentColor.a = a;
    glColor4f(r, g, b, a);
}

void Renderer::SetColor(const Color& color)
{
    m_currentColor = color;
    glColor4f(color.r, color.g, color.b, color.a);
}

void Renderer::DrawLine(const Vec3& start, const Vec3& end)
{
    glBegin(GL_LINES);
    glVertex3f(start.x, start.y, start.z);
    glVertex3f(end.x, end.y, end.z);
    glEnd();
}

void Renderer::DrawWireBox(const Vec3& center, float width, float height, float depth)
{
    float w = width * 0.5f;
    float h = height * 0.5f;
    float d = depth * 0.5f;

    glBegin(GL_LINES);
    // Bottom face
    glVertex3f(center.x - w, center.y - h, center.z - d);
    glVertex3f(center.x + w, center.y - h, center.z - d);

    glVertex3f(center.x + w, center.y - h, center.z - d);
    glVertex3f(center.x + w, center.y - h, center.z + d);

    glVertex3f(center.x + w, center.y - h, center.z + d);
    glVertex3f(center.x - w, center.y - h, center.z + d);

    glVertex3f(center.x - w, center.y - h, center.z + d);
    glVertex3f(center.x - w, center.y - h, center.z - d);

    // Top face
    glVertex3f(center.x - w, center.y + h, center.z - d);
    glVertex3f(center.x + w, center.y + h, center.z - d);

    glVertex3f(center.x + w, center.y + h, center.z - d);
    glVertex3f(center.x + w, center.y + h, center.z + d);

    glVertex3f(center.x + w, center.y + h, center.z + d);
    glVertex3f(center.x - w, center.y + h, center.z + d);

    glVertex3f(center.x - w, center.y + h, center.z + d);
    glVertex3f(center.x - w, center.y + h, center.z - d);

    // Vertical edges
    glVertex3f(center.x - w, center.y - h, center.z - d);
    glVertex3f(center.x - w, center.y + h, center.z - d);

    glVertex3f(center.x + w, center.y - h, center.z - d);
    glVertex3f(center.x + w, center.y + h, center.z - d);

    glVertex3f(center.x + w, center.y - h, center.z + d);
    glVertex3f(center.x + w, center.y + h, center.z + d);

    glVertex3f(center.x - w, center.y - h, center.z + d);
    glVertex3f(center.x - w, center.y + h, center.z + d);

    glEnd();
}

void Renderer::DrawBox(const Vec3& center, float width, float height, float depth)
{
    float w = width * 0.5f;
    float h = height * 0.5f;
    float d = depth * 0.5f;

    glBegin(GL_QUADS);
    // Front
    glNormal3f(0, 0, 1);
    glVertex3f(center.x - w, center.y - h, center.z + d);
    glVertex3f(center.x + w, center.y - h, center.z + d);
    glVertex3f(center.x + w, center.y + h, center.z + d);
    glVertex3f(center.x - w, center.y + h, center.z + d);

    // Back
    glNormal3f(0, 0, -1);
    glVertex3f(center.x + w, center.y - h, center.z - d);
    glVertex3f(center.x - w, center.y - h, center.z - d);
    glVertex3f(center.x - w, center.y + h, center.z - d);
    glVertex3f(center.x + w, center.y + h, center.z - d);

    // Top
    glNormal3f(0, 1, 0);
    glVertex3f(center.x - w, center.y + h, center.z + d);
    glVertex3f(center.x + w, center.y + h, center.z + d);
    glVertex3f(center.x + w, center.y + h, center.z - d);
    glVertex3f(center.x - w, center.y + h, center.z - d);

    // Bottom
    glNormal3f(0, -1, 0);
    glVertex3f(center.x - w, center.y - h, center.z - d);
    glVertex3f(center.x + w, center.y - h, center.z - d);
    glVertex3f(center.x + w, center.y - h, center.z + d);
    glVertex3f(center.x - w, center.y - h, center.z + d);

    // Right
    glNormal3f(1, 0, 0);
    glVertex3f(center.x + w, center.y - h, center.z + d);
    glVertex3f(center.x + w, center.y - h, center.z - d);
    glVertex3f(center.x + w, center.y + h, center.z - d);
    glVertex3f(center.x + w, center.y + h, center.z + d);

    // Left
    glNormal3f(-1, 0, 0);
    glVertex3f(center.x - w, center.y - h, center.z - d);
    glVertex3f(center.x - w, center.y - h, center.z + d);
    glVertex3f(center.x - w, center.y + h, center.z + d);
    glVertex3f(center.x - w, center.y + h, center.z - d);

    glEnd();
}

void Renderer::DrawSphere(const Vec3& center, float radius, int segments, int rings)
{
    for (int i = 0; i < rings; i++)
    {
        float phi1 = PI * i / rings;
        float phi2 = PI * (i + 1) / rings;

        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= segments; j++)
        {
            float theta = 2.0f * PI * j / segments;

            float x1 = center.x + radius * sinf(phi1) * cosf(theta);
            float y1 = center.y + radius * cosf(phi1);
            float z1 = center.z + radius * sinf(phi1) * sinf(theta);

            float x2 = center.x + radius * sinf(phi2) * cosf(theta);
            float y2 = center.y + radius * cosf(phi2);
            float z2 = center.z + radius * sinf(phi2) * sinf(theta);

            Vec3 n1 = Vec3(x1 - center.x, y1 - center.y, z1 - center.z).Normalized();
            glNormal3f(n1.x, n1.y, n1.z);
            glVertex3f(x1, y1, z1);

            Vec3 n2 = Vec3(x2 - center.x, y2 - center.y, z2 - center.z).Normalized();
            glNormal3f(n2.x, n2.y, n2.z);
            glVertex3f(x2, y2, z2);
        }
        glEnd();
    }
}

void Renderer::DrawCylinder(const Vec3& center, float radius, float height, int segments)
{
    float halfHeight = height * 0.5f;

    // Side
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= segments; i++)
    {
        float theta = 2.0f * PI * i / segments;
        float x = cosf(theta);
        float z = sinf(theta);

        glNormal3f(x, 0, z);
        glVertex3f(center.x + radius * x, center.y - halfHeight, center.z + radius * z);
        glVertex3f(center.x + radius * x, center.y + halfHeight, center.z + radius * z);
    }
    glEnd();

    // Top
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0, 1, 0);
    glVertex3f(center.x, center.y + halfHeight, center.z);
    for (int i = 0; i <= segments; i++)
    {
        float theta = 2.0f * PI * i / segments;
        glVertex3f(center.x + radius * cosf(theta), center.y + halfHeight, center.z + radius * sinf(theta));
    }
    glEnd();

    // Bottom
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0, -1, 0);
    glVertex3f(center.x, center.y - halfHeight, center.z);
    for (int i = 0; i <= segments; i++)
    {
        float theta = 2.0f * PI * i / segments;
        glVertex3f(center.x + radius * cosf(theta), center.y - halfHeight, center.z + radius * sinf(theta));
    }
    glEnd();
}

void Renderer::DrawCone(const Vec3& center, float radius, float height, int segments)
{
    float halfHeight = height * 0.5f;
    Vec3 top(center.x, center.y + halfHeight, center.z);

    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0, -1, 0);
    glVertex3f(center.x, center.y - halfHeight, center.z);
    for (int i = 0; i <= segments; i++)
    {
        float theta = 2.0f * PI * i / segments;
        float x = center.x + radius * cosf(theta);
        float z = center.z + radius * sinf(theta);

        Vec3 bottom(x, center.y - halfHeight, z);
        Vec3 n = (bottom - top).Cross(Vec3(0, -1, 0)).Normalized();
        glNormal3f(n.x, n.y, n.z);
        glVertex3f(x, center.y - halfHeight, z);
    }
    glEnd();

    // Side
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < segments; i++)
    {
        float theta1 = 2.0f * PI * i / segments;
        float theta2 = 2.0f * PI * (i + 1) / segments;

        float x1 = center.x + radius * cosf(theta1);
        float z1 = center.z + radius * sinf(theta1);
        float x2 = center.x + radius * cosf(theta2);
        float z2 = center.z + radius * sinf(theta2);

        Vec3 v1(x1, center.y - halfHeight, z1);
        Vec3 v2(x2, center.y - halfHeight, z2);

        Vec3 edge1 = v1 - top;
        Vec3 edge2 = v2 - top;
        Vec3 normal = edge2.Cross(edge1).Normalized();

        glNormal3f(normal.x, normal.y, normal.z);
        glVertex3f(top.x, top.y, top.z);
        glVertex3f(v1.x, v1.y, v1.z);
        glVertex3f(v2.x, v2.y, v2.z);
    }
    glEnd();
}

void Renderer::DrawGrid(float size, float step)
{
    glBegin(GL_LINES);
    glColor4f(0.267f, 0.267f, 0.267f, 0.5f);

    float half = size * 0.5f;
    for (float i = -half; i <= half; i += step)
    {
        glVertex3f(i, 0.01f, -half);
        glVertex3f(i, 0.01f, half);

        glVertex3f(-half, 0.01f, i);
        glVertex3f(half, 0.01f, i);
    }
    glEnd();

    glColor4f(1, 1, 1, 1);
}
