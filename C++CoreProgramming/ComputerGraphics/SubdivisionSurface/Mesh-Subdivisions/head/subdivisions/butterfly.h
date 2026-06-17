#pragma once
#include <vector>
#include <glm/glm.hpp>

namespace butterfly {
    struct vertex {
        glm::vec3 pos = glm::vec3(0.0f);
        glm::vec3 normal = glm::vec3(0.0f);
        glm::vec2 uv = glm::vec2(0.0f);

        std::vector<unsigned int> n_face;
        std::vector<unsigned int> n_vertex;
        std::vector<unsigned int> b_vertex;

        bool is_border = false;
    };

    struct face {
        unsigned int v[3] = { 0, 0, 0 };
        std::vector<unsigned int> n_edge;
    };

    struct edge {
        unsigned int v1 = 0;
        unsigned int v2 = 0;
        std::vector<unsigned int> n_face;
        bool is_border = true;
        unsigned int mid = 0;
    };
}