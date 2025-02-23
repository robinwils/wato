#pragma once
#include <glm/ext/vector_float3.hpp>

struct Transform3D {
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
};
