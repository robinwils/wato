#pragma once

#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/fwd.hpp>
#include <glm/trigonometric.hpp>
#include <renderer/primitive.hpp>

#include "glm/geometric.hpp"
#include "glm/gtc/matrix_access.hpp"
#include "glm/gtc/quaternion.hpp"
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

    [[nodiscard]] glm::vec3 Normal(const glm::quat& aOrientation) const
    {
        return glm::normalize(glm::vec3(glm::column(glm::mat4_cast(aOrientation), 1)));
    }
};
