#pragma once

#include "components/camera.hpp"
#include "glm/fwd.hpp"

struct Ray {
    Ray(glm::vec3 o, glm::dvec2 p) : orig(o), point(p) {}
    glm::vec3  orig;
    glm::dvec2 point;

    glm::vec4 word_cast(const Camera& c, float w, float h);
    glm::vec3 intersect_plane(const Camera& c, float w, float h, glm::vec3 p_normal);
};
