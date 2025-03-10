#pragma once
#include <glm/ext/vector_float3.hpp>
#include <glm/gtc/quaternion.hpp>

#include "core/physics.hpp"
#include "reactphysics3d/mathematics/Transform.h"
#include "reactphysics3d/mathematics/Vector3.h"

struct Transform3D {
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;

    rp3d::Transform to_rp3d() const
    {
        auto q = glm::quat_cast(orientation());
        return rp3d::Transform(RP3D_VEC3(position), rp3d::Quaternion(RP3D_VEC3(q), q.w));
    }

    glm::mat4 orientation() const
    {
        auto model = glm::mat4(1.0f);
        model      = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model      = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model      = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        return model;
    }
};
