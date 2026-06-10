#pragma once
#include <vector>
#include <string>

#include <glm/glm.hpp>

struct vertex_index {
    unsigned int v_idx = 0; // position index
    int vn_idx = -1; // normal index (can be -1 if no normal)
    int vt_idx = -1; // texcoords index (can be -1 if no texcoords)
};

using face_t = std::vector<vertex_index>;

struct obj_mesh {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoords;
    std::vector<face_t>    faces;
};

bool loadObj(std::string _filepath, obj_mesh & _obj);
bool writeObj(std::string _filepath, obj_mesh _obj);
