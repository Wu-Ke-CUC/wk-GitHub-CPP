#include "Model.h"
#include <iostream>
#include <cmath>

// ---- 构造函数初始化组件 ----
Model::Model() {
    this->fbx_manager = nullptr;
    this->fbx_scene = nullptr;
    this->is_fbx = false;
    this->anim_time = 0.0f;
    this->anim_duration = 0.0f;
    this->resetVars();
}

// ---- 析构函数销毁 FBX SDK 占用的底层资源 ----
Model::~Model() {
    if (this->fbx_scene) {
        this->fbx_scene->Destroy();
        this->fbx_scene = nullptr;
    }
    if (this->fbx_manager) {
        this->fbx_manager->Destroy();
        this->fbx_manager = nullptr;
    }
}

// ---- 重置用户面板的几何空间坐标控制变量 ----
void Model::resetVars() {
    this->model_x = 0;
    this->model_y = 0;
    this->model_z = 0;
    this->model_rotx = 0;
    this->model_roty = 0;
    this->model_rotz = 0;
}

// ---- 传统几何重心均值计算逻辑 ----
void Model::calculateVertex() {
    this->mean_x = 0;
    this->mean_y = 0;
    this->mean_z = 0;

    size_t numVertices = this->vertex_list.size() / 3;
    if (numVertices == 0) return;

    for (size_t q = 0; q < numVertices; q++) {
        this->mean_x += this->vertex_list.at(3 * q);
        this->mean_y += this->vertex_list.at(3 * q + 1);
        this->mean_z += this->vertex_list.at(3 * q + 2);
    }

    this->mean_x /= numVertices;
    this->mean_y /= numVertices;
    this->mean_z /= numVertices;
}

// ---- 几何AABB包围盒最大/最小值判定逻辑 ----
void Model::getMaxMins() {
    if (this->vertex_list.empty()) return;

    this->max_x = this->min_x = this->vertex_list.at(0);
    this->max_y = this->min_y = this->vertex_list.at(1);
    this->max_z = this->min_z = this->vertex_list.at(2);

    size_t numVertices = this->vertex_list.size() / 3;
    for (size_t q = 0; q < numVertices; q++) {
        float x = this->vertex_list.at(3 * q);
        float y = this->vertex_list.at(3 * q + 1);
        float z = this->vertex_list.at(3 * q + 2);

        if (x > this->max_x) this->max_x = x;
        if (x < this->min_x) this->min_x = x;
        if (y > this->max_y) this->max_y = y;
        if (y < this->min_y) this->min_y = y;
        if (z > this->max_z) this->max_z = z;
        if (z < this->min_z) this->min_z = z;
    }

    float scale_x = this->max_x - this->min_x;
    float scale_y = this->max_y - this->min_y;
    float scale_z = this->max_z - this->min_z;

    this->max_scale = scale_x;
    if (scale_y > this->max_scale) this->max_scale = scale_y;
    if (scale_z > this->max_scale) this->max_scale = scale_z;
}

// ---- 自适应归一化及视口缩放核心方法（重构兼容动画及静态模型） ----
void Model::applyTransfToMatrix() {
    this->calculateVertex();
    this->getMaxMins();

    if (this->max_scale < 0.0001f) this->max_scale = 1.0f;

    // 计算缩放比，将跨度极大的建筑或微观模型统一标准化到1.5单元左右的可见视口内
    float sFactor = 1.5f / this->max_scale;

    size_t numCoordinates = this->vertex_list.size();
    for (size_t h = 0; h < numCoordinates / 3; h++) {
        // 动态渲染数据居中平移并缩放
        this->vertex_list.at(3 * h) = (this->vertex_list.at(3 * h) - this->mean_x) * sFactor;
        this->vertex_list.at(3 * h + 1) = (this->vertex_list.at(3 * h + 1) - this->mean_y) * sFactor;
        this->vertex_list.at(3 * h + 2) = (this->vertex_list.at(3 * h + 2) - this->mean_z) * sFactor;

        // 如果包含骨骼蒙皮的绑定基础网格，采用完全一致的变换
        if (!this->base_vertex_list.empty()) {
            this->base_vertex_list.at(3 * h) = (this->base_vertex_list.at(3 * h) - this->mean_x) * sFactor;
            this->base_vertex_list.at(3 * h + 1) = (this->base_vertex_list.at(3 * h + 1) - this->mean_y) * sFactor;
            this->base_vertex_list.at(3 * h + 2) = (this->base_vertex_list.at(3 * h + 2) - this->mean_z) * sFactor;
        }
    }
}

// ---- 精确加载 FBX 骨骼动画资产核心实现 ----
bool Model::loadFBX(const std::string& filename) {
    this->is_fbx = true;
    this->anim_time = 0.0f;

    // 清空现有的数据容器
    this->vertex_list.clear();
    this->normal_list.clear();
    this->base_vertex_list.clear();
    this->base_normal_list.clear();
    this->bone_weights.clear();
    this->fbx_faces.clear();
    this->bones.clear();

    // 1. 初始化 FBX SDK 核心对象管理器
    if (!this->fbx_manager) {
        this->fbx_manager = FbxManager::Create();
        FbxIOSettings* ios = FbxIOSettings::Create(this->fbx_manager, IOSROOT);
        this->fbx_manager->SetIOSettings(ios);
    }

    FbxImporter* lImporter = FbxImporter::Create(this->fbx_manager, "");
    if (!lImporter->Initialize(filename.c_str(), -1, this->fbx_manager->GetIOSettings())) {
        return false; // 文件路径错误或格式不支持
    }

    this->fbx_scene = FbxScene::Create(this->fbx_manager, "SceneMeshAnimated");
    lImporter->Import(this->fbx_scene);
    lImporter->Destroy();

    // 2. 轴向统一重塑变换：强制将 FBX 的空间轴转换为经典的 OpenGL 规范 (Maya Y-Up 轴向)
    FbxAxisSystem sceneAxisSystem = this->fbx_scene->GetGlobalSettings().GetAxisSystem();
    FbxAxisSystem::MayaYUp.ConvertScene(this->fbx_scene);

    // 3. 递归遍历场景树，提取包含网格以及 Deformer 皮肤集群的骨骼和顶点权重数据
    this->processFBXMeshRecursive(this->fbx_scene->GetRootNode());

    if (this->vertex_list.empty()) return false;

    // 4. 重构建立完整的父子骨骼层级索引网络
    for (size_t b = 0; b < this->bones.size(); b++) {
        FbxNode* parentNode = this->bones[b].node->GetParent();
        if (parentNode) {
            for (size_t p = 0; p < this->bones.size(); p++) {
                if (this->bones[p].node == parentNode) {
                    this->bones[b].parentIndex = (int)p;
                    break;
                }
            }
        }
    }

    // 5. 读取提取当前动画栈 (Animation Stack) 持续的总生命周期周期时长
    FbxAnimStack* animStack = this->fbx_scene->GetCurrentAnimationStack();
    if (animStack) {
        FbxTimeSpan timeSpan = animStack->GetLocalTimeSpan();
        this->anim_duration = (float)timeSpan.GetDuration().GetSecondDouble();
    }
    else {
        this->anim_duration = 0.0f;
    }

    // 6. 自动化对基础模型数据执行居中对齐及包围盒映射
    this->applyTransfToMatrix();
    return true;
}

// ---- 内部私有递归提取节点网格与皮肤变形器数据方法 ----
void Model::processFBXMeshRecursive(FbxNode* node) {
    if (!node) return;

    FbxMesh* mesh = node->GetMesh();
    if (mesh) {
        // 提取骨骼或几何体本地的几何变换偏差参数 (Geometric Transform)
        FbxAMatrix geometryTransform;
        FbxVector4 lT = node->GetGeometricTranslation(FbxNode::eSourcePivot);
        FbxVector4 lR = node->GetGeometricRotation(FbxNode::eSourcePivot);
        FbxVector4 lS = node->GetGeometricScaling(FbxNode::eSourcePivot);
        geometryTransform.SetT(lT);
        geometryTransform.SetR(lR);
        geometryTransform.SetS(lS);

        int controlPointsCount = mesh->GetControlPointsCount();
        FbxVector4* controlPoints = mesh->GetControlPoints();

        // 获取底层网格关联的皮肤变形器 (Skin Deformer)
        int deformerCount = mesh->GetDeformerCount(FbxDeformer::eSkin);
        std::vector<VertexBoneData> cpWeights(controlPointsCount);

        for (int d = 0; d < deformerCount; d++) {
            FbxSkin* skin = (FbxSkin*)mesh->GetDeformer(d, FbxDeformer::eSkin);
            if (!skin) continue;

            int clusterCount = skin->GetClusterCount();
            for (int c = 0; c < clusterCount; c++) {
                FbxCluster* cluster = skin->GetCluster(c);
                FbxNode* linkNode = cluster->GetLink(); // 对应的骨骼关节实体
                if (!linkNode) continue;

                // 在全局全局骨骼数组中进行查找匹配
                int boneIdx = -1;
                for (size_t b = 0; b < this->bones.size(); b++) {
                    if (this->bones[b].node == linkNode) {
                        boneIdx = (int)b;
                        break;
                    }
                }
                // 若该骨骼关节首次被索引，建立并推入其专有的逆绑定矩阵（Inverse Bind Matrix）
                if (boneIdx == -1) {
                    FBXBone newBone;
                    newBone.node = linkNode;
                    newBone.name = linkNode->GetName();

                    FbxAMatrix transformMatrix;
                    FbxAMatrix transformLinkMatrix;
                    cluster->GetTransformMatrix(transformMatrix);         // Mesh在绑定时的全局变换
                    cluster->GetTransformLinkMatrix(transformLinkMatrix); // 骨骼在绑定时的全局变换

                    // 矩阵变换链式运算求逆：反矩阵 = 骨骼变换逆 * 网格变换 * 几何体固有变换
                    FbxAMatrix invBind = transformLinkMatrix.Inverse() * transformMatrix * geometryTransform;

                    // 拷贝保存至通用的 double[16] 平铺数组
                    memcpy(newBone.inverseBindMatrix, (const double*)invBind, sizeof(double) * 16);

                    this->bones.push_back(newBone);
                    boneIdx = (int)this->bones.size() - 1;
                }

                // 提取受该骨骼关联波及的所有控制点索引以及对应的权重因子系数
                int cpIndexCount = cluster->GetControlPointIndicesCount();
                int* cpIndices = cluster->GetControlPointIndices();
                double* cpWeightsData = cluster->GetControlPointWeights();

                for (int w = 0; w < cpIndexCount; w++) {
                    int cpIdx = cpIndices[w];
                    float weight = (float)cpWeightsData[w];
                    if (cpIdx >= 0 && cpIdx < controlPointsCount) {
                        cpWeights[cpIdx].addBoneData(boneIdx, weight);
                    }
                }
            }
        }

        // 平铺多边形多面片信息，展开为平铺的线性顶点阵列，彻底匹配原始遗留管线绘图模式
        int polygonCount = mesh->GetPolygonCount();
        int globalVertStartIdx = (int)(this->vertex_list.size() / 3);

        for (int p = 0; p < polygonCount; p++) {
            int polySize = mesh->GetPolygonSize(p);
            FBXMeshFace face;

            for (int v = 0; v < polySize; v++) {
                int cpIdx = mesh->GetPolygonVertex(p, v);
                FbxVector4 vertexPos = controlPoints[cpIdx];

                // 推入渲染和基准姿态几何数据流
                this->vertex_list.push_back((float)vertexPos[0]);
                this->vertex_list.push_back((float)vertexPos[1]);
                this->vertex_list.push_back((float)vertexPos[2]);

                this->base_vertex_list.push_back((float)vertexPos[0]);
                this->base_vertex_list.push_back((float)vertexPos[1]);
                this->base_vertex_list.push_back((float)vertexPos[2]);

                // 索取法线矢量
                FbxVector4 normal(0.0, 1.0, 0.0, 0.0);
                mesh->GetPolygonVertexNormal(p, v, normal);
                this->normal_list.push_back((float)normal[0]);
                this->normal_list.push_back((float)normal[1]);
                this->normal_list.push_back((float)normal[2]);

                this->base_normal_list.push_back((float)normal[0]);
                this->base_normal_list.push_back((float)normal[1]);
                this->base_normal_list.push_back((float)normal[2]);

                // 压入本顶点对应的蒙皮权重
                this->bone_weights.push_back(cpWeights[cpIdx]);

                face.vertexIndices.push_back(globalVertStartIdx++);
            }
            this->fbx_faces.push_back(face);
        }
    }

    // 递归下发子节点扫描
    for (int i = 0; i < node->GetChildCount(); i++) {
        this->processFBXMeshRecursive(node->GetChild(i));
    }
}

// ---- 时间轮询驱动步进：重塑骨骼节点矩阵状态 ----
void Model::updateAnimation(float deltaTime) {
    if (!this->is_fbx || this->bones.empty()) return;

    // 动画播放时刻推进
    this->anim_time += deltaTime;
    if (this->anim_duration > 0.0f) {
        if (this->anim_time > this->anim_duration) {
            this->anim_time = fmod(this->anim_time, this->anim_duration); // 循环往复播放
        }
    }

    FbxTime fbxTime;
    fbxTime.SetSecondDouble((double)this->anim_time);

    // 获取动画采样器求值器实例 (Animation Evaluator)
    FbxAnimEvaluator* evaluator = this->fbx_scene->GetAnimationEvaluator();

    for (size_t b = 0; b < this->bones.size(); b++) {
        // 求得当前时间帧关节的绝对全局动画变换矩阵
        FbxAMatrix currentGlobalTransform = evaluator->GetNodeGlobalTransform(this->bones[b].node, fbxTime);

        // 恢复获取反逆矩阵
        FbxAMatrix invBind;
        memcpy((double*)invBind, this->bones[b].inverseBindMatrix, sizeof(double) * 16);

        // 级联相乘，拼装为最终的蒙皮肌肉驱动矩阵 M_skin = CurrentGlobalTransform * InverseBindMatrix
        FbxAMatrix skinningMatrix = currentGlobalTransform * invBind;
        memcpy(this->bones[b].currentTransform, (const double*)skinningMatrix, sizeof(double) * 16);
        FbxVector4 globalPos = currentGlobalTransform.GetT();
        this->bones[b].currentGlobalPos[0] = globalPos[0];
        this->bones[b].currentGlobalPos[1] = globalPos[1];
        this->bones[b].currentGlobalPos[2] = globalPos[2];
    }

    // 矩阵重塑完毕，立即下发 CPU 线性混合蒙皮，更新重算所有网格顶点
    this->computeSkinning();
}

// ---- 核心多骨骼线性混合蒙皮算法 (Linear Blend Skinning, LBS) CPU 实现 ----
void Model::computeSkinning() {
    size_t vertexCount = this->base_vertex_list.size() / 3;

    for (size_t i = 0; i < vertexCount; i++) {
        // 基准姿态顶点的初始坐标
        float bx = this->base_vertex_list[3 * i];
        float by = this->base_vertex_list[3 * i + 1];
        float bz = this->base_vertex_list[3 * i + 2];

        // 基准法线朝向
        float bnx = this->base_normal_list[3 * i];
        float bny = this->base_normal_list[3 * i + 1];
        float bnz = this->base_normal_list[3 * i + 2];

        float outX = 0.0f, outY = 0.0f, outZ = 0.0f;
        float outNX = 0.0f, outNY = 0.0f, outNZ = 0.0f;

        const VertexBoneData& wData = this->bone_weights[i];

        // 归一化校验：求算有效影响权重之和，避免非对称剪裁浮点数累计误差
        float totalWeight = 0.0f;
        for (int k = 0; k < 4; k++) {
            if (wData.boneIDs[k] != -1) totalWeight += wData.weights[k];
        }
        if (totalWeight < 0.001f) totalWeight = 1.0f;

        bool hasSkinning = false;
        for (int k = 0; k < 4; k++) {
            int bIdx = wData.boneIDs[k];
            float weight = wData.weights[k] / totalWeight;

            if (bIdx >= 0 && bIdx < (int)this->bones.size() && weight > 0.0f) {
                hasSkinning = true;
                const double* M = this->bones[bIdx].currentTransform;

                // 1. 对顶点坐标执行矩阵乘法与权重混叠（平移项生效）
                outX += (float)(M[0] * bx + M[4] * by + M[8] * bz + M[12]) * weight;
                outY += (float)(M[1] * bx + M[5] * by + M[9] * bz + M[13]) * weight;
                outZ += (float)(M[2] * bx + M[6] * by + M[10] * bz + M[14]) * weight;

                // 2. 对法线矢量执行矩阵乘法（舍弃第4列平移项，仅做纯方向姿态改变）
                outNX += (float)(M[0] * bnx + M[4] * bny + M[8] * bnz) * weight;
                outNY += (float)(M[1] * bnx + M[5] * bny + M[9] * bnz) * weight;
                outNZ += (float)(M[2] * bnx + M[6] * bny + M[10] * bnz) * weight;
            }
        }

        // 安全回滚降级：针对完全没有骨骼附加的流浪顶点，维持原始姿态
        if (!hasSkinning) {
            outX = bx; outY = by; outZ = bz;
            outNX = bnx; outNY = bny; outNZ = bnz;
        }

        // 3. 将解算出的最终动态几何蒙皮坐标送回 OpenGL 渲染管线内存区
        this->vertex_list[3 * i] = outX;
        this->vertex_list[3 * i + 1] = outY;
        this->vertex_list[3 * i + 2] = outZ;

        // 单位正交化重算，规避缩放导致的法线不均
        float len = sqrtf(outNX * outNX + outNY * outNY + outNZ * outNZ);
        if (len > 0.0001f) {
            outNX /= len; outNY /= len; outNZ /= len;
        }
        this->normal_list[3 * i] = outNX;
        this->normal_list[3 * i + 1] = outNY;
        this->normal_list[3 * i + 2] = outNZ;
    }
}
