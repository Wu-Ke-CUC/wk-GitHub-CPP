// Scene.cpp - Scene Implementation
#include "Include/stdafx.h"
#include "Include/Scene.h"
#include "Include/Renderer.h"

extern Renderer* g_renderer;

Scene::Scene() : m_skyQuadric(nullptr)
{
}

Scene::~Scene()
{
    if (m_skyQuadric)
    {
        gluDeleteQuadric(m_skyQuadric);
    }
}

void Scene::Initialize()
{
    srand((unsigned int)time(nullptr));

    m_skyQuadric = gluNewQuadric();
    gluQuadricOrientation(m_skyQuadric, GLU_INSIDE);

    CreateGround();
    CreateTrees();
    CreateMountains();
    CreateHouses();
    CreateGrid();
}

void Scene::CreateGround()
{
}

void Scene::CreateTrees()
{
    int treeCount = 30;
    m_trees.reserve(treeCount);

    for (int i = 0; i < treeCount; i++)
    {
        Tree tree;
        tree.position.x = Math::RandRange(-90.0f, 90.0f);
        tree.position.z = Math::RandRange(-90.0f, 90.0f);
        tree.position.y = 0;

        if (fabsf(tree.position.x) < 20.0f && fabsf(tree.position.z) < 20.0f)
        {
            i--;
            continue;
        }

        m_trees.push_back(tree);
    }
}

void Scene::CreateMountains()
{
    int mountainCount = 5;
    m_mountains.reserve(mountainCount);

    for (int i = 0; i < mountainCount; i++)
    {
        Mountain mountain;
        mountain.height = Math::RandRange(20.0f, 50.0f);
        mountain.radius = Math::RandRange(30.0f, 70.0f);

        float angle = (float)i / mountainCount * 2.0f * PI;
        float distance = 120.0f;
        mountain.position.x = cosf(angle) * distance;
        mountain.position.z = sinf(angle) * distance;
        mountain.position.y = 0;  // Place on ground

        m_mountains.push_back(mountain);
    }
}

void Scene::CreateHouses()
{
    int houseCount = 3;
    m_houses.reserve(houseCount);

    for (int i = 0; i < houseCount; i++)
    {
        House house;
        house.width = Math::RandRange(10.0f, 30.0f);
        house.depth = Math::RandRange(10.0f, 30.0f);
        house.wallHeight = Math::RandRange(10.0f, 30.0f);
        house.roofHeight = Math::RandRange(5.0f, 10.0f);

        float angle = (float)i / houseCount * 2.0f * PI;
        float distance = Math::RandRange(30.0f, 60.0f);
        house.position.x = cosf(angle) * distance;
        house.position.z = sinf(angle) * distance;
        house.position.y = 0;

        m_houses.push_back(house);
    }
}

void Scene::CreateSkybox()
{
}

void Scene::CreateGrid()
{
}

void Scene::Render()
{
    glPushMatrix();

    // Draw ground
    glColor4f(0.486f, 0.988f, 0.0f, 1.0f);
    glBegin(GL_TRIANGLES);
    glNormal3f(0, 1, 0);
    glVertex3f(-100, 0, -100);
    glVertex3f(100, 0, -100);
    glVertex3f(100, 0, 100);

    glVertex3f(-100, 0, -100);
    glVertex3f(100, 0, 100);
    glVertex3f(-100, 0, 100);
    glEnd();

    // Draw grid
    glColor4f(0.267f, 0.267f, 0.267f, 0.5f);
    glBegin(GL_LINES);
    float halfSize = 100.0f;
    float step = 4.0f;
    for (float i = -halfSize; i <= halfSize; i += step)
    {
        glVertex3f(i, 0.01f, -halfSize);
        glVertex3f(i, 0.01f, halfSize);

        glVertex3f(-halfSize, 0.01f, i);
        glVertex3f(halfSize, 0.01f, i);
    }
    glEnd();

    // Draw trees
    for (const auto& tree : m_trees)
    {
        glPushMatrix();
        glTranslatef(tree.position.x, tree.position.y, tree.position.z);

        // Trunk
        glColor4f(0.545f, 0.271f, 0.075f, 1.0f);
        g_renderer->DrawCylinder(Vec3(0, 1, 0), 0.3f, 2.0f, 8);

        // Crown
        glColor4f(0.133f, 0.545f, 0.133f, 1.0f);
        g_renderer->DrawCone(Vec3(0, 3, 0), 1.5f, 3.0f, 8);

        glPopMatrix();
    }

    // Draw mountains
    for (const auto& mountain : m_mountains)
    {
        glPushMatrix();
        glTranslatef(mountain.position.x, mountain.position.y, mountain.position.z);

        glColor4f(0.5f, 0.5f, 0.5f, 1.0f);
        glBegin(GL_TRIANGLES);

        int segments = 8;
        for (int i = 0; i < segments; i++)
        {
            float theta1 = 2.0f * PI * i / segments;
            float theta2 = 2.0f * PI * (i + 1) / segments;

            glVertex3f(0, mountain.height, 0);
            glVertex3f(mountain.radius * cosf(theta1), 0, mountain.radius * sinf(theta1));
            glVertex3f(mountain.radius * cosf(theta2), 0, mountain.radius * sinf(theta2));
        }

        glEnd();

        glPopMatrix();
    }

    //Draw houses
    for (const auto& house : m_houses)
    {
        glPushMatrix();
        glTranslatef(house.position.x, house.position.y, house.position.z);

        // 绘制墙体（四个竖直面，使用 GL_QUADS）
        glColor4f(0.7f, 0.5f, 0.3f, 1.0f);  // 砖墙色
        float halfW = house.width * 0.5f;
        float halfD = house.depth * 0.5f;
        float yTop = house.wallHeight;

        glBegin(GL_QUADS);
        // 前面（z = +halfD）
        glVertex3f(-halfW, 0.0f, halfD);
        glVertex3f(halfW, 0.0f, halfD);
        glVertex3f(halfW, yTop, halfD);
        glVertex3f(-halfW, yTop, halfD);
        // 后面（z = -halfD）
        glVertex3f(-halfW, 0.0f, -halfD);
        glVertex3f(-halfW, yTop, -halfD);
        glVertex3f(halfW, yTop, -halfD);
        glVertex3f(halfW, 0.0f, -halfD);
        // 左面（x = -halfW）
        glVertex3f(-halfW, 0.0f, -halfD);
        glVertex3f(-halfW, 0.0f, halfD);
        glVertex3f(-halfW, yTop, halfD);
        glVertex3f(-halfW, yTop, -halfD);
        // 右面（x = +halfW）
        glVertex3f(halfW, 0.0f, halfD);
        glVertex3f(halfW, 0.0f, -halfD);
        glVertex3f(halfW, yTop, -halfD);
        glVertex3f(halfW, yTop, halfD);
        glEnd();

        // 绘制屋顶（四个三角形，构成四棱锥）
        glColor4f(0.8f, 0.2f, 0.1f, 1.0f);  // 红色屋顶
        float roofTopY = house.wallHeight + house.roofHeight;
        glBegin(GL_TRIANGLES);
        // 前面屋顶
        glVertex3f(-halfW, yTop, halfD);
        glVertex3f(halfW, yTop, halfD);
        glVertex3f(0.0f, roofTopY, 0.0f);
        // 后面屋顶
        glVertex3f(-halfW, yTop, -halfD);
        glVertex3f(0.0f, roofTopY, 0.0f);
        glVertex3f(halfW, yTop, -halfD);
        // 左面屋顶
        glVertex3f(-halfW, yTop, -halfD);
        glVertex3f(-halfW, yTop, halfD);
        glVertex3f(0.0f, roofTopY, 0.0f);
        // 右面屋顶
        glVertex3f(halfW, yTop, halfD);
        glVertex3f(halfW, yTop, -halfD);
        glVertex3f(0.0f, roofTopY, 0.0f);
        glEnd();

        glPopMatrix();
    }

    // Draw sky sphere
    glColor4f(0.529f, 0.808f, 0.922f, 1.0f);
    glPushMatrix();
    glTranslatef(0, 0, 0);
    glScalef(500.0f, 500.0f, 500.0f);
    glFrontFace(GL_CW);
    gluSphere(m_skyQuadric, 1.0, 32, 32);
    glFrontFace(GL_CCW);
    glPopMatrix();

    glPopMatrix();
}

void Scene::Update(float deltaTime)
{
}
