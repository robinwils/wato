#pragma once

#include "glm/fwd.hpp"
glm::vec4 ray_cast(const Camera& cam, float w, float h, glm::dvec2 orig);
