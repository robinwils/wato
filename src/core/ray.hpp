#pragma once

#include "components/camera.hpp"
#include "glm/fwd.hpp"

struct Ray {
    Ray(const Camera& c, glm::vec3 o, glm::dvec2 p) : camera(c), orig(o), point(p) {}
    glm::vec3     orig;
    glm::dvec2    point;
    const Camera& camera;

    glm::vec4 word_cast(float w, float h);
};
