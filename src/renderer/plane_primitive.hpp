#pragma once

#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/fwd.hpp>
#include <glm/trigonometric.hpp>
#include <renderer/primitive.hpp>

#include "glm/geometric.hpp"
#include "glm/gtc/matrix_access.hpp"
#include "renderer/material.hpp"

class PlanePrimitive : public Primitive<PositionNormalUvVertex>
{
   public:
    explicit PlanePrimitive(Material* aMaterial) : Primitive<PositionNormalUvVertex>(aMaterial)
    {
        mVertices = {
            {{+0.5F, +0.0F, -0.5F}, {0.0F, +1.0F, 0.0F}, {0.0F, 1.0F}},
            {{-0.5F, +0.0F, -0.5F}, {0.0F, +1.0F, 0.0F}, {0.0F, 0.0F}},
            {{-0.5F, +0.0F, +0.5F}, {0.0F, +1.0F, 0.0F}, {1.0F, 0.0F}},
            {{+0.5F, +0.0F, +0.5F}, {0.0F, +1.0F, 0.0F}, {1.0F, 1.0F}},
        };

        mIndices = {
            0,
            1,
            2,  //

            0,
            2,
            3,  //
        };

        Primitive::InitializePrimitive();
    }

    [[nodiscard]] glm::vec3 Normal(const glm::vec3& aRotation) const
    {
        auto model = glm::mat4(1.0F);
        model      = glm::rotate(model, glm::radians(aRotation.x), glm::vec3(1.0F, 0.0F, 0.0F));
        model      = glm::rotate(model, glm::radians(aRotation.y), glm::vec3(0.0F, 1.0F, 0.0F));
        model      = glm::rotate(model, glm::radians(aRotation.z), glm::vec3(0.0F, 0.0F, 1.0F));

        return glm::normalize(glm::vec3(glm::column(model, 1)));
    }
};
