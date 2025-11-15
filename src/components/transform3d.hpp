#pragma once

#include <reactphysics3d/reactphysics3d.h>

#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/fwd.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/trigonometric.hpp>

#include "core/serialize.hpp"
#include "core/sys/log.hpp"

struct Transform3D {
    glm::vec3 Position{0.0f};
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

    bool Archive(auto& aArchive)
    {
        if (!ArchiveVector(aArchive, Position, 0.0f, 20.0f)) return false;
        if (!ArchiveQuaternion(aArchive, Orientation)) return false;
        if (!ArchiveVector(aArchive, Scale, 0.0f, 20.0f)) return false;
        return true;
    }
};

template <>
struct fmt::formatter<Transform3D> : fmt::formatter<std::string> {
    auto format(const Transform3D& aObj, format_context& aCtx) const -> decltype(aCtx.out())
    {
        return fmt::format_to(
            aCtx.out(),
            "Position = {}, Orientation = {}, Scale = {}",
            aObj.Position,
            aObj.Orientation,
            aObj.Scale);
    }
};
