#pragma once
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/fwd.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/trigonometric.hpp>
#include <string>

#include "reactphysics3d/reactphysics3d.h"

struct Transform3D {
    glm::vec3 Position;
    glm::vec3 Rotation;
    glm::vec3 Scale;

    [[nodiscard]] rp3d::Transform ToRp3d() const
    {
        auto q = glm::quat_cast(Orientation());
        return rp3d::Transform(rp3d::Vector3(Position.x, Position.y, Position.z),
            rp3d::Quaternion(q.x, q.y, q.z, q.w));
    }

    [[nodiscard]] glm::mat4 Orientation() const
    {
        auto model = glm::mat4(1.0F);
        model      = glm::rotate(model, glm::radians(Rotation.x), glm::vec3(1.0F, 0.0F, 0.0F));
        model      = glm::rotate(model, glm::radians(Rotation.y), glm::vec3(0.0F, 1.0F, 0.0F));
        model      = glm::rotate(model, glm::radians(Rotation.z), glm::vec3(0.0F, 0.0F, 1.0F));
        return model;
    }

    [[nodiscard]] std::string ToString() const
    {
        return "Transform3D: position = " + glm::to_string(Position)
               + ", rotation = " + glm::to_string(Rotation) + ", scale = " + glm::to_string(Scale);
    }
};
