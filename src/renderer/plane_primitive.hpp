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
    explicit PlanePrimitive(std::unique_ptr<Material> aMaterial)
        : Primitive<PositionNormalUvVertex>(
              {
                  {{+1.0F, +0.0F, -0.0F}, {0.0F, +1.0F, 0.0F}, {0.0F, 1.0F}},
                  {{-0.0F, +0.0F, -0.0F}, {0.0F, +1.0F, 0.0F}, {0.0F, 0.0F}},
                  {{-0.0F, +0.0F, +1.0F}, {0.0F, +1.0F, 0.0F}, {1.0F, 0.0F}},
                  {{+1.0F, +0.0F, +1.0F}, {0.0F, +1.0F, 0.0F}, {1.0F, 1.0F}}
    },
              {
                  0,
                  1,
                  2,
                  0,
                  2,
                  3,
              },
              std::move(aMaterial))
    {
    }

    [[nodiscard]] glm::vec3 Normal(const glm::quat& aOrientation) const
    {
        return glm::normalize(glm::vec3(glm::column(glm::mat4_cast(aOrientation), 1)));
    }
};
