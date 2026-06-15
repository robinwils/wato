#pragma once

#include <bgfx/bgfx.h>

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

    [[nodiscard]] glm::mat4 Projection(float aFov, float aAspect) const
    {
        // [-1,1]  GL
        // [0,1]   VK/D3D/Metal
        return bgfx::getCaps()->homogeneousDepth
                   ? glm::perspectiveRH_NO(glm::radians(aFov), aAspect, NearClip, FarClip)
                   : glm::perspectiveRH_ZO(glm::radians(aFov), aAspect, NearClip, FarClip);
    }
    [[nodiscard]] glm::mat4 Projection(float aAspect) const { return Projection(Fov, aAspect); }

    [[nodiscard]] glm::mat4 View(glm::vec3 aPos) const { return glm::lookAt(aPos, aPos + Dir, Up); }
};
