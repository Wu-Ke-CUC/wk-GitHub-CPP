/* Class Model */
#ifndef Model_H
#define Model_H

#include <vector>
#include <string>
#include <fstream>

// 引入 Autodesk FBX SDK 头文件
#include <fbxsdk.h>

/**
 * @brief 单个顶点的骨骼绑定与权重数据结构（最大支持 4 骨骼混合蒙皮）
 */
struct VertexBoneData {
    int boneIDs[4] = { -1, -1, -1, -1 };
    float weights[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    // 向当前顶点中安全地压入一根影响骨骼
    void addBoneData(int boneID, float weight) {
        for (int i = 0; i < 4; i++) {
            if (boneIDs[i] == -1) {
                boneIDs[i] = boneID;
                weights[i] = weight;
                return;
            }
        }
    }
};

/**
 * @brief 骨骼关节节点信息结构体
 */
struct FBXBone {
    fbxsdk::FbxNode* node = nullptr; // 对应 FBX SDK 中的底层节点指针
    std::string name;                // 骨骼关节名称
    int parentIndex = -1;            // 父关节索引，-1 表示根节点
    double inverseBindMatrix[16];    // 4x4 逆绑定矩阵 (Inverse Bind Matrix)，将顶点自模型空间变换到骨骼本地空间
    double currentTransform[16];     // 当前帧的最终皮肤变换复合矩阵 (GlobalTransform * InverseBindMatrix)
    double currentGlobalPos[3];      // 当前帧骨骼关节在世界空间中的绝对位置 (x, y, z)
};

/**
 * @brief 拓扑面片结构体，用以兼容 FBX 复杂多边形的分离排布
 */
struct FBXMeshFace {
    std::vector<int> vertexIndices;  // 存储该面片拥有的所有顶点在全局渲染列表中的对应索引
};

class Model
{
public:
    // ---- 构造与析构函数 ----
    Model();
    ~Model();

    // ---- 通用基础控制函数 ----
    void resetVars();                     // 重置所有变换控制变量
    void calculateVertex();               // 计算顶点的中心均值（用以居中对齐）
    void getMaxMins();                    // 提取边界体最大/最小值（用以自适应缩放）
    void applyTransfToMatrix();           // 对顶点数据执行居中归一化及视口缩放映射

    // ---- 骨骼动画核心功能扩展 ----
    bool loadFBX(const std::string& filename);   // 解析并载入带有骨骼动画信息的 FBX 模型文件
    void updateAnimation(float deltaTime);        // 时间驱动步进：更新骨骼姿态并重塑肌肉网格
    void computeSkinning();                       // CPU 端核心算法：线性混合蒙皮 (Linear Blend Skinning) 实时计算

    // ---- 通用动态几何数据缓存（供 OpenGL 实时绘制） ----
    std::vector<float> vertex_list;          // 目标渲染顶点流数据 (X, Y, Z 排布，动态更新)
    std::vector<float> normal_list;          // 目标渲染法线流数据 (NX, NY, NZ 排布，动态更新)
    std::vector<std::string> s_list;         // 存储静态 OBJ 格式的面描述行文本

    // ---- 归一化中心与比例属性 ----
    float mean_x;
    float mean_y;
    float mean_z;
    float max_x, min_x;
    float max_y, min_y;
    float max_z, min_z;
    float max_scale;

    // ---- 面板交互按钮映射的模型变换变量 ----
    float model_x;
    float model_y;
    float model_z;
    float model_rotx;
    float model_roty;
    float model_rotz;

    // ---- FBX 状态与动画信息资产 ----
    bool is_fbx;                             // 当前是否为动画 FBX 模式
    float anim_time;                         // 动画当前播放到的时间戳（秒）
    float anim_duration;                     // 动画总时长（秒）

    std::vector<float> base_vertex_list;     // 原始静态绑定姿态（Bind Pose）下的顶点基准坐标
    std::vector<float> base_normal_list;     // 原始静态绑定姿态下的法线基准方向
    std::vector<float> bind_vertex_orig;     // 原始绑定姿态顶点（未归一化）
    std::vector<float> bind_normal_orig;     // 原始绑定姿态法线（未归一化）
    std::vector<VertexBoneData> bone_weights;// 顶点对应的骨骼关联信息（与 base_vertex_list 一一对应）
    std::vector<FBXBone> bones;              // 全局骨骼层级节点树平铺列表
    std::vector<FBXMeshFace> fbx_faces;      // 记录面片网格的拓扑信息

private:
    // ---- FBX 底层 SDK 管理器指针 ----
    fbxsdk::FbxManager* fbx_manager;
    fbxsdk::FbxScene* fbx_scene;

    // ---- 递归辅助函数 ----
    void processFBXMeshRecursive(fbxsdk::FbxNode* node); // 递归遍历骨骼节点网络提取网格和蒙皮权重
};

#endif