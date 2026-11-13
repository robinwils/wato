#pragma once

#include <entt/entt.hpp>
#include <glm/ext/vector_double2.hpp>
#include <glm/ext/vector_float3.hpp>

struct PickState {
    std::optional<glm::vec3>  MouseWorldIntersect;
    std::optional<glm::dvec2> SelectStartPos;
    std::optional<glm::dvec2> SelectCurPos;
    std::optional<glm::dvec2> SelectEndPos;

    bool IsInSelection(glm::dvec2 aScreen)
    {
        return aScreen.x >= SelectStartPos->x && aScreen.x <= SelectEndPos->x
               && aScreen.y >= SelectStartPos->y && aScreen.y <= SelectEndPos->y;
    }
};
