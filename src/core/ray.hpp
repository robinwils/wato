#pragma once

#include "glm/ext/vector_float3.hpp"
#include "glm/geometric.hpp"

struct Ray {
    Ray(glm::vec3 o, glm::vec3 d) : orig(o), dir(d) {}
    glm::vec3 orig;
    glm::vec3 dir;

    float intersect_plane(const glm::vec3& p_normal) const
    {
        return glm::dot(orig, p_normal) / glm::dot(dir, p_normal);
    }
};
