#pragma once
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>

struct Camera {
    glm::vec3 up;
    glm::vec3 front;
    glm::vec3 dir;
    float     speed;
    float     fov;
    float     near_clip;
    float     far_clip;

    glm::vec3 right() const { return glm::cross(up, front); }
};
