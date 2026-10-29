#pragma once

#include <entt/entt.hpp>
#include <glm/ext/vector_double2.hpp>
#include <glm/ext/vector_float3.hpp>

struct PickState {
    std::optional<glm::vec3>  MouseWorldIntersect;
    std::optional<glm::dvec2> SelectStartPos;
    std::optional<glm::dvec2> SelectCurPos;
};
