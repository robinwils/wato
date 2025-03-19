#pragma once

#include <glm/ext/matrix_clip_space.hpp>  // glm::perspective
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>

struct Camera {
    glm::vec3 Up;
    glm::vec3 Front;
    glm::vec3 Dir;
    float     Speed;
    float     Fov;
    float     NearClip;
    float     FarClip;

    [[nodiscard]] glm::vec3 Right() const { return glm::cross(Up, Front); }
    [[nodiscard]] glm::mat4 Projection(float aW, float aH) const
    {
        return glm::perspective(glm::radians(Fov), aW / aH, NearClip, FarClip);
    }
    [[nodiscard]] glm::mat4 View(glm::vec3 aPos) const { return glm::lookAt(aPos, aPos + Dir, Up); }
};
