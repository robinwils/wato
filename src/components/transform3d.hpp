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
    glm::quat Orientation;
    glm::vec3 Scale;

    [[nodiscard]] rp3d::Transform ToRP3D() const
    {
        return rp3d::Transform(rp3d::Vector3(Position.x, Position.y, Position.z),
            rp3d::Quaternion(Orientation.x, Orientation.y, Orientation.z, Orientation.w));
    }

    [[nodiscard]] std::string ToString() const
    {
        return "Transform3D: position = " + glm::to_string(Position) + ", rotation = "
               + glm::to_string(Orientation) + ", scale = " + glm::to_string(Scale);
    }

    void FromRP3D(const rp3d::Transform& aTransform)
    {
        Position.x = aTransform.getPosition().x;
        Position.y = aTransform.getPosition().y;
        Position.z = aTransform.getPosition().z;

        Orientation = glm::quat(aTransform.getOrientation().w,
            aTransform.getOrientation().x,
            aTransform.getOrientation().y,
            aTransform.getOrientation().z);
    }
};
