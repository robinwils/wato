#pragma once

#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/fwd.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/trigonometric.hpp>
#include <string>

#include "reactphysics3d/reactphysics3d.h"

struct Transform3D {
    glm::vec3 Position;
    glm::quat Orientation{glm::identity<glm::quat>()};
    glm::vec3 Scale{1.0f};

    [[nodiscard]] rp3d::Transform ToRP3D() const
    {
        return rp3d::Transform(
            rp3d::Vector3(Position.x, Position.y, Position.z),
            rp3d::Quaternion(Orientation.x, Orientation.y, Orientation.z, Orientation.w));
    }

    void FromRP3D(const rp3d::Transform& aTransform)
    {
        Position.x = aTransform.getPosition().x;
        Position.y = aTransform.getPosition().y;
        Position.z = aTransform.getPosition().z;

        Orientation = glm::quat(
            aTransform.getOrientation().w,
            aTransform.getOrientation().x,
            aTransform.getOrientation().y,
            aTransform.getOrientation().z);
    }

    [[nodiscard]] glm::mat4 ModelMat() const
    {
        return glm::translate(glm::identity<glm::mat4>(), Position) * glm::mat4_cast(Orientation)
               * glm::scale(glm::identity<glm::mat4>(), Scale);
    }

    constexpr static auto Serialize(auto& aArchive, const auto& aSelf)
    {
        aArchive.template Write<float>(glm::value_ptr(aSelf.Position), 3);
        aArchive.template Write<float>(glm::value_ptr(aSelf.Orientation), 4);
        aArchive.template Write<float>(glm::value_ptr(aSelf.Scale), 3);
    }

    constexpr static auto Deserialize(auto& aArchive, auto& aSelf)
    {
        aArchive.template Read<float>(glm::value_ptr(aSelf.Position), 3);
        aArchive.template Read<float>(glm::value_ptr(aSelf.Orientation), 4);
        aArchive.template Read<float>(glm::value_ptr(aSelf.Scale), 3);
        return true;
    }
};
