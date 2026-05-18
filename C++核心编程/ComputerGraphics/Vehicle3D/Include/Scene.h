// Scene.h - Scene
#pragma once

#include "Math3D.h"
#include <vector>

struct Tree
{
    Vec3 position;
};

struct Mountain
{
    Vec3 position;
    float height;
    float radius;
};

//房子结构体
struct House
{
    Vec3 position;
    float width;
    float depth;
    float wallHeight;
    float roofHeight;
};

class Scene
{
public:
    Scene();
    ~Scene();

    void Initialize();
    void Render();
    void Update(float deltaTime);

private:
    void CreateGround();
    void CreateTrees();
    void CreateMountains();
    void CreatHouses();
    void CreateSkybox();
    void CreateGrid();

    std::vector<Tree> m_trees;
    std::vector<Mountain> m_mountains;
    std::vector<House> m_houses;

    GLUquadric* m_skyQuadric;
};
