#pragma once

#include <glm/ext/matrix_clip_space.hpp>  // glm::perspective
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>

struct Camera {
    glm::vec3 up;
    glm::vec3 front;
    glm::vec3 dir;
    float     speed;
    float     fov;
    float     near_clip;
    float     far_clip;

    glm::vec3 right() const { return glm::cross(up, front); }
    glm::mat4 proj(float _w, float _h) const
    {
        return glm::perspective(glm::radians(fov), _w / _h, near_clip, far_clip);
    }
    glm::mat4 view(glm::vec3 pos) const { return glm::lookAt(pos, pos + dir, up); }
};
