#include "HermiteCurve.h"

// ==================== TODO 1: Hermite三次基函数 ====================
// 功能：输入参数 t (0~1)，返回四个Hermite三次基函数值
//   h00(t) = 2t³ - 3t² + 1    — P0 的位置权重
//   h10(t) = t³ - 2t² + t     — P0 的切向量权重
//   h01(t) = -2t³ + 3t²       — P1 的位置权重
//   h11(t) = t³ - t²          — P1 的切向量权重
HermiteBasis hermiteBasis(float t) {
    // TODO: 实现Hermite三次基函数计算
    float t2 = t * t;
    float t3 = t2 * t;
    HermiteBasis basis;
    basis.h00 = 2 * t3 - 3 * t2 + 1;
    basis.h10 = t3 - 2 * t2 + t;
    basis.h01 = -2 * t3 + 3 * t2;
    basis.h11 = t3 - t2;
    return basis;
}

// ==================== TODO 2: Hermite插值 ====================
// 功能：在两个关键帧之间进行Hermite三次插值，返回 t 时刻的插值点坐标
// 参数：
//   kf0, kf1 — 相邻两个关键帧（包含位置、时间、切向量）
//   t — 局部插值参数 (0~1)，t=0 对应 kf0，t=1 对应 kf1
// 实现要点：
//   1. 调用 hermiteBasis(t) 获取基函数 h00/h10/h01/h11
//   2. 计算时间差 dt = kf1.time - kf0.time，保护 dt>0
//   3. 切向量需乘以 dt，将 dP/d(time) 转换为 dP/dt_local 尺度
//   4. 插值公式：
//      P(t) = h00*P0 + h10*T0*dt + h01*P1 + h11*T1*dt
Point2f interpolateHermite(const Keyframe& kf0, const Keyframe& kf1, float t) {
    // TODO: 实现两点间Hermite插值
     // 1. 获取当前局部参数 t 对应的四个基函数值
    HermiteBasis basis = hermiteBasis(t);
    // 2. 计算局部时间区间长度（用于缩放切向量）
    float dt = kf1.time - kf0.time;
    if (dt <= 0.0f) {
        dt = 1.0f;  // 防御性保护：若关键帧时间未正确排序，强制设为 1
    }
    // 3. 应用 Hermite 插值公式计算 X 坐标
    //    注意：tangentX 必须乘以 dt，实现从全局时间到局部参数的映射
    float x = basis.h00 * kf0.x + basis.h10 * kf0.tangentX * dt + basis.h01 * kf1.x + basis.h11 * kf1.tangentX * dt;
    // 4. 同理计算 Y 坐标
    float y = basis.h00 * kf0.y + basis.h10 * kf0.tangentY * dt + basis.h01 * kf1.y + basis.h11 * kf1.tangentY * dt;
    return Point2f(x, y);
}

// ==================== TODO 3: 生成整条曲线采样点 ====================
// 功能：遍历所有相邻关键帧对，逐段采样生成完整曲线点集
// 参数：
//   keyframes — 已排序的关键帧数组（至少2个，每个含位置/时间/切向量）
//   segmentsPerSegment — 每段采样分辨率（默认50）
// 实现要点：
//   1. 遍历 keyframes[i] 到 keyframes[i+1] 的每一对
//   2. 对每段均匀采样 segmentsPerSegment+1 个点（含两端）
//   3. 调用 interpolateHermite(kf0, kf1, t) 获取每个采样点
std::vector<Point2f> generateCurvePoints(
    const std::vector<Keyframe>& keyframes,
    int segmentsPerSegment)
{
    // TODO: 实现遍历所有关键帧对生成完整曲线采样点集
    std::vector<Point2f> points;
    // 边界检查：关键帧少于 2 个或采样精度非法，返回空集合
    if (keyframes.size() < 2 || segmentsPerSegment <= 0)
        return points;
    // 遍历每一段关键帧区间
    for (size_t i = 0; i < keyframes.size() - 1; ++i) {
        const Keyframe& kf0 = keyframes[i];
        const Keyframe& kf1 = keyframes[i + 1];
        // 在该段内均匀采样，j 从 0 到 segmentsPerSegment
        // 当 j == segmentsPerSegment 时，t = 1.0，对应下一段的起始点（会被下一段覆盖采样，此处保留以闭合曲线）
        for (int j = 0; j <= segmentsPerSegment; ++j) {
            float t = (float)j / segmentsPerSegment;
            points.push_back(interpolateHermite(kf0, kf1, t));
        }
    }
    return points;
}

// ==================== 排序关键帧并重新均匀分配时间（已实现） ====================
void sortKeyframes(std::vector<Keyframe>& keyframes) {
    if (keyframes.size() <= 1) return;
    std::sort(keyframes.begin(), keyframes.end(),
        [](const Keyframe& a, const Keyframe& b) { return a.time < b.time; });
    for (size_t i = 0; i < keyframes.size(); i++) {
        keyframes[i].time = (float)i / (keyframes.size() - 1);
    }
}

// ==================== TODO 4: 生成经过所有关键帧的平滑Hermite曲线 ====================
// 功能：对已排序的关键帧，计算切向量并用Hermite插值生成完整曲线
// 参数：
//   keyframes — 关键帧数组（输入输出，切向量会被写回用于箭头显示）
//   screenWidth, screenHeight — 画布像素尺寸（用于归一化/反归一化）
//   segmentsPerSpan — 每段采样精度（默认200）
//
// 实现步骤（三Steps）：
//   Step 1 — 归一化位置到 [0,1] 范围：
//     np[i].x = keyframes[i].x / screenWidth
//     np[i].y = keyframes[i].y / screenHeight
//
//   Step 2 — 计算每个关键帧的切向量（dP/d(time) 尺度）：
//     分支判断：keyframes[i].tangentUserEdited 标志
//     * 未编辑（false）：自动计算 Catmull-Rom 切向量
//       - 首帧：单侧差分  (np[1] - np[0]) / (time[1] - time[0])
//       - 尾帧：单侧差分  (np[N-1] - np[N-2]) / (time[N-1] - time[N-2])
//       - 中间帧：中心差分 (np[i+1] - np[i-1]) / (time[i+1] - time[i-1])
//     * 已编辑（true）：保留用户方向，长度使用CR计算的幅度
//       归一化用户方向 × CR切向量长度 → 写回
//     写回 keyframes[i].tangentX/Y 用于箭头显示（乘以 screenWidth/Height）
//
//   Step 3 — Hermite插值生成采样点：
//     遍历每对相邻关键帧，调用 hermiteBasis(t) 获取基函数
//     插值公式：P(t) = h00*P0 + h10*T0*dt + h01*P1 + h11*T1*dt
//     注意所有切向量需 ×dt（转为 dP/dt_local），最后反归一化
std::vector<Point2f> generateCurveThroughPoints(
    std::vector<Keyframe>& keyframes,
    float screenWidth, float screenHeight,
    int segmentsPerSpan)
{
    // TODO: 实现完整的三步骤Hermite曲线生成算法
    std::vector<Point2f> points;
    size_t n = keyframes.size();
    // 基本校验：至少 2 个关键帧，画布尺寸有效，采样精度有效
    if (n < 2 || screenWidth <= 0 || screenHeight <= 0 || segmentsPerSpan <= 0)
        return points;
    // ==================== Step 1: 归一化 ====================
    // 创建归一化副本用于计算，避免破坏原始屏幕坐标（但会覆盖其切向量）
    std::vector<Keyframe> normKfs = keyframes;
    for (auto& kf : normKfs) {
        kf.x /= screenWidth;
        kf.y /= screenHeight;
    }
    // ==================== Step 2: 计算切向量 ====================
    for (size_t i = 0; i < n; ++i) {
        // 默认 CR 切向量（归一化空间）
        float crTx = 0.0f, crTy = 0.0f;
        // --- 2.1 自动计算 Catmull-Rom 型切向量 ---
        if (i == 0) {
            // 首帧：前向单侧差分
            float dt = normKfs[1].time - normKfs[0].time;
            if (dt > 0) {
                crTx = (normKfs[1].x - normKfs[0].x) / dt;
                crTy = (normKfs[1].y - normKfs[0].y) / dt;
            }
        }
        else if (i == n - 1) {
            // 尾帧：后向单侧差分
            float dt = normKfs[n - 1].time - normKfs[n - 2].time;
            if (dt > 0) {
                crTx = (normKfs[n - 1].x - normKfs[n - 2].x) / dt;
                crTy = (normKfs[n - 1].y - normKfs[n - 2].y) / dt;
            }
        }
        else {
            // 中间帧：中心差分（最准确）
            float dt = normKfs[i + 1].time - normKfs[i - 1].time;
            if (dt > 0) {
                crTx = (normKfs[i + 1].x - normKfs[i - 1].x) / dt;
                crTy = (normKfs[i + 1].y - normKfs[i - 1].y) / dt;
            }
        }
        // --- 2.2 处理用户手动编辑的切向量 ---
        if (keyframes[i].tangentUserEdited) {
            // 将用户编辑的屏幕切向量转换到归一化空间
            float udx = keyframes[i].tangentX / screenWidth;
            float udy = keyframes[i].tangentY / screenHeight;
            float ulen = sqrtf(udx * udx + udy * udy);
            if (ulen > 0.001f) {
                // 情况 A：用户方向有效 -> 保持方向，幅度采用 CR 长度
                // 归一化用户方向向量
                udx /= ulen;
                udy /= ulen;
                // 计算 CR 长度
                float crLen = sqrtf(crTx * crTx + crTy * crTy);
                if (crLen > 0.001f) {
                    // 将 CR 长度赋予用户方向
                    crTx = udx * crLen;
                    crTy = udy * crLen;
                }
                // 若 CR 长度接近 0（直线穿越或重合点），保留 CR 方向（即零向量）
                // 但这里 crTx/Ty 保持原 CR 值（可能为零），不影响逻辑
            }
            else {
                // 情况 B：用户方向长度太小（接近零）-> 完全使用 CR 向量
                // crTx/Ty 已计算好，无需操作
            }
        } // 若未编辑，直接使用 crTx/Ty
        // --- 2.3 存储归一化切向量到副本，并写回屏幕空间切向量 ---
        normKfs[i].tangentX = crTx;
        normKfs[i].tangentY = crTy;
        // 写回原始关键帧（乘以屏幕尺寸），用于 GL 渲染箭头
        keyframes[i].tangentX = crTx * screenWidth;
        keyframes[i].tangentY = crTy * screenHeight;
    }

    // ==================== Step 3: Hermite 插值生成采样点 ====================
    for (size_t i = 0; i < n - 1; ++i) {
        const Keyframe& kf0 = normKfs[i];
        const Keyframe& kf1 = normKfs[i + 1];
        for (int j = 0; j <= segmentsPerSpan; ++j) {
            float t = (float)j / segmentsPerSpan;
            // 在归一化空间进行插值
            Point2f normP = interpolateHermite(kf0, kf1, t);
            // 反归一化到屏幕坐标并输出
            points.push_back(Point2f(normP.x * screenWidth, normP.y * screenHeight));
        }
    }

    return points;
}

// ==================== 折线段线性插值位置（已实现） ====================
Point2f getPolylinePosition(const std::vector<Keyframe>& keyframes, float animTime) {
    if (keyframes.empty()) return Point2f();
    if (keyframes.size() == 1) return Point2f(keyframes[0].x, keyframes[0].y);

    for (size_t i = 0; i < keyframes.size() - 1; i++) {
        if (animTime >= keyframes[i].time && animTime <= keyframes[i + 1].time) {
            float dt = keyframes[i + 1].time - keyframes[i].time;
            float localT = (dt > 0) ? (animTime - keyframes[i].time) / dt : 0;
            return Point2f(
                keyframes[i].x + (keyframes[i + 1].x - keyframes[i].x) * localT,
                keyframes[i].y + (keyframes[i + 1].y - keyframes[i].y) * localT
            );
        }
    }

    if (animTime <= keyframes[0].time)
        return Point2f(keyframes[0].x, keyframes[0].y);
    return Point2f(keyframes.back().x, keyframes.back().y);
}

// ==================== TODO 6: 获取动画位置（Hermite曲线插值） ====================
// 功能：Hermite曲线模式下，根据动画时间 animTime 获取运动物体在曲线上的位置
// 参数：
//   keyframes — 已排序的关键帧数组
//   animTime — 全局动画时间 (0~1)
// 实现要点：
//   1. 定位 animTime 落在哪两个关键帧之间
//   2. 计算局部插值参数 t = (animTime - kf0.time) / (kf1.time - kf0.time)
//   3. 调用 interpolateHermite(kf0, kf1, t) 得到曲线上的位置
Point2f getAnimPosition(const std::vector<Keyframe>& keyframes, float animTime) {
    // TODO: 实现Hermite曲线插值定位
    if (keyframes.empty()) return Point2f();
    if (keyframes.size() == 1) return Point2f(keyframes[0].x, keyframes[0].y);
    // 遍历查找匹配区间
    for (size_t i = 0; i < keyframes.size() - 1; ++i) {
        if (animTime >= keyframes[i].time && animTime <= keyframes[i + 1].time) {
            float dt = keyframes[i + 1].time - keyframes[i].time;
            // 防止除零错误（虽然关键帧时间已排序均匀，但加一层保护）
            if (dt <= 0) return Point2f(keyframes[i].x, keyframes[i].y);
            float localT = (animTime - keyframes[i].time) / dt;
            return interpolateHermite(keyframes[i], keyframes[i + 1], localT);
        }
    }
    // 边界处理：若 animTime 小于首帧或大于尾帧，返回对应的端点位置
    if (animTime <= keyframes[0].time)
        return Point2f(keyframes[0].x, keyframes[0].y);
    return Point2f(keyframes.back().x, keyframes.back().y);
}

// ==================== 根据模式获取动画位置（已实现） ====================
// 折线段模式（0）已实现；Hermite曲线模式（1）需完成 getAnimPosition()
Point2f getAnimPositionByMode(const std::vector<Keyframe>& keyframes,
    float animTime, int trajectoryMode)
{
    if (trajectoryMode == 0) {
        return getPolylinePosition(keyframes, animTime);
    } else {
        // TODO 学生: 完成 getAnimPosition() 后此分支自动生效
        return getAnimPosition(keyframes, animTime);
    }
}
