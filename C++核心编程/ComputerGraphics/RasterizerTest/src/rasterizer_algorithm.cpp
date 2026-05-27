#include "rasterizer_algorithm.h"

float TriangleSignedArea2(const IntPoint triangle[3])
{
    // TODO: 三角形面积的2倍
    float Ax = triangle[0].x, Ay = triangle[0].y;
    float Bx = triangle[1].x, By = triangle[1].y;
    float Cx = triangle[2].x, Cy = triangle[2].y;
    return (Bx - Ax) * (Cy - Ay) - (By - Ay) * (Cx - Ax);
}

void myBuildEdgeEquations(const IntPoint& p1, const IntPoint& p2, EdgeEquation& edge)
{
    // 边方程 a*x + b*y + c = 0，使得对边上的点值为0
    // 取 (a, b) 为从 p1 到 p2 的垂直方向（法线）
    edge.a = static_cast<float>(p1.y - p2.y);
    edge.b = static_cast<float>(p2.x - p1.x);
    edge.c = static_cast<float>(p1.x * p2.y - p2.x * p1.y);
}
void BuildEdgeEquations(const IntPoint triangle[3], EdgeEquation edges[3])
{
    // TODO: 三条边的边方程
    myBuildEdgeEquations(triangle[0], triangle[1], edges[0]);
    myBuildEdgeEquations(triangle[1], triangle[2], edges[1]);
    myBuildEdgeEquations(triangle[2], triangle[0], edges[2]);
}


bool IsDegenerateTriangle(const IntPoint triangle[3])
{
    // TODO: 为了避免求面积比例时出现被0除问题，三角形的面积小于1e-5时，认为三角形退化
    float area2 = TriangleSignedArea2(triangle);
    return fabs(area2) < 1e-5f;
}

BoundingBox CalculateBoundingBox(const IntPoint triangle[3])
{
    // TODO: 计算三角形的轴向包围盒
    BoundingBox box;
    box.minX = box.maxX = triangle[0].x;
    box.minY = box.maxY = triangle[0].y;
    for (int i = 1; i < 3; ++i)
    {
        if (triangle[i].x < box.minX) box.minX = triangle[i].x;
        if (triangle[i].x > box.maxX) box.maxX = triangle[i].x;
        if (triangle[i].y < box.minY) box.minY = triangle[i].y;
        if (triangle[i].y > box.maxY) box.maxY = triangle[i].y;
    }
    box.valid = true;
    return box;
}

bool EvaluateInside(const EdgeEquation edges[3], float x, float y)
{
    float dummy[3]{};
    return EvaluateInside(edges, x, y, dummy);
}

bool EvaluateInside(const EdgeEquation edges[3], float x, float y, float values[3])
{
    // TODO: 判别(x, y)点是否位于三角形内，同时在values中保存三个边方程的数值
    for (int i = 0; i < 3; ++i)
        values[i] = edges[i].Evaluate(x, y);

    const float epsilon = 1e-6f;
    bool allNonNegative = (values[0] >= -epsilon && values[1] >= -epsilon && values[2] >= -epsilon);
    bool allNonPositive = (values[0] <= epsilon && values[1] <= epsilon && values[2] <= epsilon);
    return allNonNegative || allNonPositive;
}

FloatColor InterpolateColor(const IntPoint triangle[3], const FloatColor vertexColors[3], float x, float y)
{
    // TODO: 根据三个顶点的颜色，对三角形内(x, y)点进行颜色插值

    // 将顶点坐标转换为浮点数
    float Ax = static_cast<float>(triangle[0].x);
    float Ay = static_cast<float>(triangle[0].y);
    float Bx = static_cast<float>(triangle[1].x);
    float By = static_cast<float>(triangle[1].y);
    float Cx = static_cast<float>(triangle[2].x);
    float Cy = static_cast<float>(triangle[2].y);

    // 计算三角形的有向面积（2倍）
    float area2_ABC = (Bx - Ax) * (Cy - Ay) - (By - Ay) * (Cx - Ax);
    if (fabs(area2_ABC) < 1e-6f)
    {
        // 退化三角形，返回默认灰色
        return MakeColor(0.70f, 0.70f, 0.70f);
    }

    // 计算三个子三角形的有向面积（2倍）
    float area2_PBC = (Bx - x) * (Cy - y) - (By - y) * (Cx - x);
    float area2_PCA = (Cx - x) * (Ay - y) - (Cy - y) * (Ax - x);
    float area2_PAB = (Ax - x) * (By - y) - (Ay - y) * (Bx - x);

    // 计算重心坐标
    float alpha = area2_PBC / area2_ABC;   // 对应顶点 A
    float beta = area2_PCA / area2_ABC;   // 对应顶点 B
    float gamma = area2_PAB / area2_ABC;   // 对应顶点 C

    // 颜色插值
    float r = alpha * vertexColors[0].r + beta * vertexColors[1].r + gamma * vertexColors[2].r;
    float g = alpha * vertexColors[0].g + beta * vertexColors[1].g + gamma * vertexColors[2].g;
    float b = alpha * vertexColors[0].b + beta * vertexColors[1].b + gamma * vertexColors[2].b;

    return MakeColor(r, g, b);
}
