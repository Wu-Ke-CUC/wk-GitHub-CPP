#include <vector>
#include <map>
#include <algorithm>
#include <iostream>
#include <glm/glm.hpp>

#include "subdivision.h"

using namespace butterfly;

void ButterflySubdivision::loadMesh(obj_mesh& _obj) {
    edgeList.clear(); faceList.clear(); vertexList.clear();
    std::map<std::pair<int, int>, int> edgeIndex;
    vertexList.resize(_obj.positions.size());

    for (unsigned int i = 0; i < _obj.faces.size(); i += 1) {
        auto curFace = _obj.faces[i];
        unsigned int faceIdx = (unsigned int)faceList.size();
        face f;

        if (curFace.size() != 3) {
            std::cout << "failed to apply butterfly subdivision on non-triangle mesh" << std::endl;
            edgeList.clear(); faceList.clear(); vertexList.clear();
            return;
        }

        for (unsigned int j = 0; j <= 2; j += 1) {
            unsigned int vi = curFace[j].v_idx;
            int vn = curFace[j].vn_idx;

            f.v[j] = vi;
            glm::vec3 pos = _obj.positions[vi];
            glm::vec3 normal = (vn != -1) ? _obj.normals[vn] : glm::vec3(0.0f);

            vertexList[vi].pos = pos;
            vertexList[vi].normal += normal;
            vertexList[vi].n_face.push_back(faceIdx);
            vertexList[vi].is_border = false;
        }

        for (unsigned int j = 0; j <= 2; j += 1) {
            unsigned int edgeIdx, mini, maxi;
            mini = std::min(f.v[j], f.v[(j + 1) % 3]);
            maxi = std::max(f.v[j], f.v[(j + 1) % 3]);
            auto key = std::pair<int, int>(mini, maxi);

            if (edgeIndex.find(key) == edgeIndex.end()) {
                edgeIdx = (int)edgeList.size();
                edge l;
                l.v1 = key.first; l.v2 = key.second;
                edgeList.push_back(std::move(l));
                edgeIndex[key] = edgeIdx;
                edgeList[edgeIdx].is_border = true;
            }
            else {
                edgeIdx = edgeIndex[key];
                edgeList[edgeIdx].is_border = false;
            }
            edgeList[edgeIdx].n_face.push_back(faceIdx);
            f.n_edge.push_back(edgeIdx);
        }
        faceList.push_back(f);
    }

    for (auto curEdge = edgeList.cbegin(); curEdge != edgeList.end(); curEdge++) {
        if (curEdge->is_border) {
            vertexList[curEdge->v1].is_border = true;
            vertexList[curEdge->v2].is_border = true;
            vertexList[curEdge->v1].b_vertex.push_back(curEdge->v2);
            vertexList[curEdge->v2].b_vertex.push_back(curEdge->v1);
        }
        else {
            vertexList[curEdge->v1].n_vertex.push_back(curEdge->v2);
            vertexList[curEdge->v2].n_vertex.push_back(curEdge->v1);
        }
    }

    for (auto curVertex = vertexList.begin(); curVertex != vertexList.end(); curVertex++) {
        if (glm::length(curVertex->normal) > 1e-5f) {
            curVertex->normal = glm::normalize(curVertex->normal);
        }
    }
}

void ButterflySubdivision::subdiv() {
    std::vector<vertex> oldVertexList(vertexList);
    std::vector<edge> oldEdgeList(edgeList);
    std::vector<face> oldFaceList(faceList);

    vertexList.clear(); edgeList.clear(); faceList.clear();
    vertexList.resize(oldVertexList.size());

    // 辅助函数：寻找通过指定边共享的对面顶点 (8点模板的延伸点)
    auto findOpposite = [&](unsigned int va, unsigned int vb, unsigned int face_exclude) -> int {
        for (unsigned int f_idx : oldVertexList[va].n_face) {
            if (f_idx == face_exclude) continue;
            face f = oldFaceList[f_idx];
            if (f.v[0] == vb || f.v[1] == vb || f.v[2] == vb) {
                for (int i = 0; i < 3; ++i) {
                    if (f.v[i] != va && f.v[i] != vb) return f.v[i];
                }
            }
        }
        return -1;
        };

    // 1. 生成新顶点 (插值细分核心逻辑)
    for (auto curEdge = oldEdgeList.begin(); curEdge != oldEdgeList.end(); curEdge++) {
        vertex v0 = oldVertexList[curEdge->v1];
        vertex v1 = oldVertexList[curEdge->v2];
        vertex mid;
        int midIdx = (int)vertexList.size();

        if (curEdge->is_border) {
            // 边界：中点退化
            mid.pos = 0.5f * v0.pos + 0.5f * v1.pos;
            mid.normal = glm::normalize(0.5f * v0.normal + 0.5f * v1.normal);
            mid.is_border = true;
        }
        else {
            // 内部：标准 Butterfly 8点模板
            unsigned int f0_idx = curEdge->n_face[0];
            unsigned int f1_idx = curEdge->n_face[1];

            unsigned int v0_idx = curEdge->v1;
            unsigned int v1_idx = curEdge->v2;
            unsigned int v2_idx = 0, v3_idx = 0;

            // 找到两个相邻面上相对的顶点 v2, v3
            for (int k = 0; k < 3; k++) {
                if (oldFaceList[f0_idx].v[k] != v0_idx && oldFaceList[f0_idx].v[k] != v1_idx) v2_idx = oldFaceList[f0_idx].v[k];
                if (oldFaceList[f1_idx].v[k] != v0_idx && oldFaceList[f1_idx].v[k] != v1_idx) v3_idx = oldFaceList[f1_idx].v[k];
            }

            // 寻找外围的 4 个延伸点 v4, v5, v6, v7
            int v4_idx = findOpposite(v0_idx, v2_idx, f0_idx);
            int v5_idx = findOpposite(v1_idx, v2_idx, f0_idx);
            int v6_idx = findOpposite(v0_idx, v3_idx, f1_idx);
            int v7_idx = findOpposite(v1_idx, v3_idx, f1_idx);

            // 如果是规则网格且能找到所有8个点，应用完整的 Butterfly 权重
            if (v4_idx >= 0 && v5_idx >= 0 && v6_idx >= 0 && v7_idx >= 0) {
                mid.pos = 0.5f * (v0.pos + v1.pos)
                    + 0.125f * (oldVertexList[v2_idx].pos + oldVertexList[v3_idx].pos)
                    - 0.0625f * (oldVertexList[v4_idx].pos + oldVertexList[v5_idx].pos + oldVertexList[v6_idx].pos + oldVertexList[v7_idx].pos);
            }
            else {
                // 如果附近有边界缺失点，退化为简单的 1/2 + 1/8 权重（避免越界）
                mid.pos = 0.5f * (v0.pos + v1.pos) + 0.125f * (oldVertexList[v2_idx].pos + oldVertexList[v3_idx].pos);
            }

            // 法线简单插值
            mid.normal = glm::normalize(0.5f * (v0.normal + v1.normal));
            mid.is_border = false;
        }
        vertexList.push_back(std::move(mid));
        curEdge->mid = midIdx;
    }

    // 2. Butterfly 是插值细分，旧顶点不移动！这是它与 Loop 最大的区别。
    for (unsigned int vi = 0; vi < oldVertexList.size(); vi += 1) {
        vertexList[vi].pos = oldVertexList[vi].pos;
        vertexList[vi].normal = oldVertexList[vi].normal;
        vertexList[vi].is_border = oldVertexList[vi].is_border;
    }

    edgeList.resize(oldEdgeList.size() * 2);
    faceList.resize(oldFaceList.size() * 4);

    // 3. 构建新的面和内边 (拓扑拆分逻辑与 Loop 一致)
    for (unsigned int fi = 0; fi < oldFaceList.size(); fi += 1) {
        face cur = oldFaceList[fi];
        edge old_edges[] = { oldEdgeList[cur.n_edge[0]], oldEdgeList[cur.n_edge[1]], oldEdgeList[cur.n_edge[2]] };

        unsigned int inner_newedges_index[3];
        edge inner_newedges[3];
        unsigned int outer_newedges_index[3][2];
        edge outer_newedges[3][2];

        unsigned int outer_newfaces_index[3] = { 4 * fi, 4 * fi + 1, 4 * fi + 2 };
        unsigned int inner_newfaces_index = 4 * fi + 3;

        for (unsigned int i = 0; i <= 2; i += 1) {
            inner_newedges[i].v1 = old_edges[i].mid;
            inner_newedges[i].v2 = (i != 0) ? old_edges[i - 1].mid : old_edges[2].mid;
            inner_newedges[i].n_face.push_back(4 * fi + i);
            inner_newedges[i].n_face.push_back(4 * fi + 3);
            inner_newedges[i].is_border = false;
            inner_newedges_index[i] = (int)edgeList.size();
            edgeList.push_back(inner_newedges[i]);
        }

        for (unsigned int i = 0; i <= 2; i += 1) {
            outer_newedges[i][0].v1 = cur.v[i];
            outer_newedges[i][0].v2 = old_edges[i].mid;
            outer_newedges[i][1].v1 = old_edges[i].mid;
            outer_newedges[i][1].v2 = (i != 2) ? cur.v[i + 1] : cur.v[0];

            if (outer_newedges[i][0].v1 < outer_newedges[i][1].v2) {
                outer_newedges_index[i][0] = 2 * cur.n_edge[i];
                outer_newedges_index[i][1] = 2 * cur.n_edge[i] + 1;
            }
            else {
                outer_newedges_index[i][0] = 2 * cur.n_edge[i] + 1;
                outer_newedges_index[i][1] = 2 * cur.n_edge[i];
            }

            for (unsigned int j = 0; j <= 1; j += 1) {
                edge* l = &edgeList[outer_newedges_index[i][j]];
                l->v1 = outer_newedges[i][j].v1;
                l->v2 = outer_newedges[i][j].v2;
                l->is_border = old_edges[i].is_border;
                l->n_face.push_back((i + j != 3) ? (4 * fi + i + j) : (4 * fi));
            }
        }

        for (unsigned int i = 0; i <= 2; i += 1) {
            face* f = &faceList[outer_newfaces_index[i]];
            f->v[0] = cur.v[i];
            f->v[1] = old_edges[i].mid;
            f->n_edge.push_back(outer_newedges_index[i][0]);
            f->n_edge.push_back(inner_newedges_index[i]);
            if (i != 0) {
                f->v[2] = old_edges[i - 1].mid;
                f->n_edge.push_back(outer_newedges_index[i - 1][1]);
            }
            else {
                f->v[2] = old_edges[2].mid;
                f->n_edge.push_back(outer_newedges_index[2][1]);
            }
        }

        face* inner_face = &faceList[inner_newfaces_index];
        for (unsigned int i = 0; i <= 2; i += 1) {
            inner_face->v[i] = old_edges[i].mid;
            inner_face->n_edge.push_back((i != 2) ? inner_newedges_index[i + 1] : inner_newedges_index[0]);
        }
    }

    // 重新建立邻接关系
    for (auto curEdge = edgeList.cbegin(); curEdge != edgeList.end(); curEdge++) {
        if (curEdge->is_border) {
            vertexList[curEdge->v1].is_border = true;
            vertexList[curEdge->v2].is_border = true;
            vertexList[curEdge->v1].b_vertex.push_back(curEdge->v2);
            vertexList[curEdge->v2].b_vertex.push_back(curEdge->v1);
        }
        else {
            vertexList[curEdge->v1].n_vertex.push_back(curEdge->v2);
            vertexList[curEdge->v2].n_vertex.push_back(curEdge->v1);
        }
    }
}

obj_mesh ButterflySubdivision::makeMesh() {
    obj_mesh res;
    res.positions.reserve(vertexList.size());
    for (auto iter = vertexList.cbegin(); iter != vertexList.end(); iter++) {
        res.positions.push_back(iter->pos);
    }
    res.faces.reserve(faceList.size());
    for (auto iter = faceList.cbegin(); iter != faceList.end(); iter++) {
        face_t newFace;
        for (unsigned int i = 0; i < 3; i += 1) {
            vertex_index vi;
            vi.v_idx = iter->v[i];
            vi.vn_idx = -1;
            vi.vt_idx = -1;
            newFace.push_back(vi);
        }
        res.faces.push_back(newFace);
    }
    return res;
}