#pragma once

#include <vector>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ==================== 基础结构 ====================

struct Point2f {
    float x, y;
    Point2f() : x(0), y(0) {}
    Point2f(float _x, float _y) : x(_x), y(_y) {}
};

// 关键帧类（25px命中半径）
class Keyframe {
public:
    float x, y;          // 画布坐标
    float time;          // 时间参数 0~1
    float tangentX, tangentY;  // 屏幕空间导数 dP/dt（用于Hermite插值）
    bool selected;
    bool tangentUserEdited;    // 用户手动编辑过切向量 → 重新生成曲线时保留

    Keyframe() : x(0), y(0), time(0), tangentX(0), tangentY(-1),
                 selected(false), tangentUserEdited(false) {}
    Keyframe(float _x, float _y, float _time)
        : x(_x), y(_y), time(_time), tangentX(0), tangentY(-1),
          selected(false), tangentUserEdited(false) {}

    // 获取箭头终点（scale=箭头视觉长度，自动归一化方向）
    Point2f GetArrowEnd(float scale = 60.0f) const {
        float len = sqrtf(tangentX * tangentX + tangentY * tangentY);
        if (len < 0.001f) return Point2f(x, y - scale);  // 默认朝上
        return Point2f(x + tangentX / len * scale, y + tangentY / len * scale);
    }

    // 25px命中检测
    bool ContainsPoint(float px, float py) const {
        float dx = px - x, dy = py - y;
        return (dx * dx + dy * dy) <= 625.0f;  // 25² = 625
    }
};

// Hermite三次基函数结果
struct HermiteBasis {
    float h00, h10, h01, h11;
};

// ==================== 函数声明 ====================

// TODO 1: Hermite三次基函数
// 输入 t (0~1)，返回四个基函数值：
//   h00(t) = 2t³ - 3t² + 1    — P0 的位置权重
//   h10(t) = t³ - 2t² + t     — P0 的切向量权重
//   h01(t) = -2t³ + 3t²       — P1 的位置权重
//   h11(t) = t³ - t²          — P1 的切向量权重
// 实现位置：HermiteCurve.cpp → hermiteBasis()
HermiteBasis hermiteBasis(float t);

// TODO 2: 两点间Hermite三次插值
// 参数：kf0, kf1 — 相邻关键帧；t — 局部参数 (0~1)
// 要点：调用 hermiteBasis(t)；计算 dt；P(t) = h00*P0 + h10*T0*dt + h01*P1 + h11*T1*dt
// 实现位置：HermiteCurve.cpp → interpolateHermite()
Point2f interpolateHermite(const Keyframe& kf0, const Keyframe& kf1, float t);

// TODO 3: 遍历关键帧对，逐段生成整条曲线采样点集
// 参数：keyframes — 已排序关键帧数组；segmentsPerSegment — 每段采样数（默认50）
// 实现位置：HermiteCurve.cpp → generateCurvePoints()
std::vector<Point2f> generateCurvePoints(
    const std::vector<Keyframe>& keyframes,
    int segmentsPerSegment = 50);

// 排序关键帧并按时间均匀分配（已实现，无需修改）
void sortKeyframes(std::vector<Keyframe>& keyframes);

// TODO 4（核心综合题）: 三步骤生成经过所有关键帧的平滑Hermite曲线
// 参数：keyframes — 关键帧（切向量会写回）；screenWidth/Height — 画布尺寸
// 步骤：
//   Step1 - 归一化位置到 [0,1]
//   Step2 - Catmull-Rom切向量（首尾单侧/中间中心差分），处理 tangentUserEdited
//   Step3 - Hermite插值 + 反归一化输出
// 实现位置：HermiteCurve.cpp → generateCurveThroughPoints()
std::vector<Point2f> generateCurveThroughPoints(
    std::vector<Keyframe>& keyframes,
    float screenWidth, float screenHeight,
    int segmentsPerSpan = 200);

// 折线段线性插值（已实现，无需修改）
Point2f getPolylinePosition(const std::vector<Keyframe>& keyframes, float animTime);

// TODO 6: Hermite曲线模式下的动画位置插值
// 参数：keyframes — 已排序关键帧；animTime — 全局动画时间 (0~1)
// 要点：定位 animTime 所在区间 → 计算 localT → 调用 interpolateHermite()
// 实现位置：HermiteCurve.cpp → getAnimPosition()
Point2f getAnimPosition(const std::vector<Keyframe>& keyframes, float animTime);

// 根据轨迹模式获取动画位置
// 模式0=折线段(已实现)；模式1=Hermite曲线(需完成getAnimPosition)
Point2f getAnimPositionByMode(const std::vector<Keyframe>& keyframes,
    float animTime, int trajectoryMode);

// ==================== 交互状态枚举 ====================
enum class EditStep {
    Idle,
    Adding,         // 新增关键帧流程
    EditingPos,     // 编辑位置（拖拽中）
    EditingTangent  // 编辑切向量（移动鼠标调箭头，点击确认）
};
