#pragma once
#include <glm/ext/vector_float3.hpp>

#include "core/physics.hpp"
#include "reactphysics3d/mathematics/Transform.h"

struct Transform3D {
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;

    rp3d::Transform& to_rp3d() const
    {
        return rp3d::Transform(RP3D_VEC3(position),
            rp3d::Matrix3x3(decimal a1,
                decimal             a2,
                decimal             a3,
                decimal             b1,
                decimal             b2,
                decimal             b3,
                decimal             c1,
                decimal             c2,
                decimal             c3))
    }
};
