#pragma once

#include <entt/entt.hpp>
#include <glm/ext/vector_float3.hpp>

struct PickState {
    std::optional<glm::vec3> MouseWorldIntersect;
};
