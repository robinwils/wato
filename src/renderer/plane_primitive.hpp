#pragma once

#include <renderer/primitive.hpp>

#include "components/transform3d.hpp"
#include "glm/geometric.hpp"
#include "glm/gtc/matrix_access.hpp"

class PlanePrimitive : public Primitive
{
   public:
    PlanePrimitive(const Material& _material) : Primitive(_material)
    {
        m_vertices = {
            {{+0.5f, +0.0f, -0.5f}, {0.0f, +1.0f, 0.0f}, {0.0f, 1.0f}},
            {{-0.5f, +0.0f, -0.5f}, {0.0f, +1.0f, 0.0f}, {0.0f, 0.0f}},
            {{-0.5f, +0.0f, +0.5f}, {0.0f, +1.0f, 0.0f}, {1.0f, 0.0f}},
            {{+0.5f, +0.0f, +0.5f}, {0.0f, +1.0f, 0.0f}, {1.0f, 1.0f}},
        };

        m_indices = {
            0,
            1,
            2,  //

            0,
            2,
            3,  //
        };

        Primitive::initializePrimitive();
    }

    glm::vec3 normal(const glm::vec3& rotation) const
    {
        auto model = glm::mat4(1.0f);
        model      = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model      = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model      = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

        return glm::normalize(glm::vec3(glm::column(model, 1)));
    }
};
