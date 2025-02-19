#pragma once

#include <glm/ext.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>

#include "input/input.hpp"

class Camera
{
   public:
    glm::vec3 m_position = glm::vec3(0.0f, 2.0f, 1.5f);
    glm::vec3 m_dir      = glm::vec3(0.0f, -1.0f, -1.0f);
    glm::vec3 m_front    = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 m_up       = glm::vec3(0.0f, 1.0f, 0.0f);  // world coordinates

    float m_speed     = 2.5f;
    float m_fov       = 60.0f;
    float m_near_clip = 0.1f;
    float m_far_clip  = 100.0f;

    glm::vec3 right() const { return glm::cross(m_up, m_front); }

    // matrix to transform any vector into camera coordinate space
    glm::mat4 view() const { return glm::lookAt(m_position, m_position + m_dir, m_up); }
    glm::mat4 projection(float _aspect) const
    {
        return glm::perspective(glm::radians(m_fov), _aspect, m_near_clip, m_far_clip);
    }

    void drawImgui();
    void update(const Input& _input, double _timeDelta);
};
