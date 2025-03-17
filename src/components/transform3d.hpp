#pragma once
#include <glm/ext/vector_float3.hpp>
#include <glm/fwd.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>

#include "reactphysics3d/reactphysics3d.h"

struct Transform3D {
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;

    rp3d::Transform to_rp3d() const
    {
        auto q = glm::quat_cast(orientation());
        return rp3d::Transform(rp3d::Vector3(position.x, position.y, position.z),
            rp3d::Quaternion(q.x, q.y, q.z, q.w));
    }

    glm::mat4 orientation() const
    {
        auto model = glm::mat4(1.0f);
        model      = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model      = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model      = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        return model;
    }

    std::string to_string() const
    {
        return "Transform3D: position = " + glm::to_string(position)
               + ", rotation = " + glm::to_string(rotation) + ", scale = " + glm::to_string(scale);
    }
};
