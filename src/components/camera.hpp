#pragma once
#include <glm/ext/vector_float3.hpp>

struct Camera {
    glm::vec3 up;
    glm::vec3 front;
    glm::vec3 dir;
    float     speed;
    float     fov;
    float     near_clip;
    float     far_clip;
};
