#pragma once

#include <map>
#include <optional>
#include <string>

#include "components/animator.hpp"
#include "components/creep.hpp"
#include "components/scene_object.hpp"
#include "components/tower.hpp"
#include "components/tower_attack.hpp"
#include "components/transform3d.hpp"
#include "core/physics/physics.hpp"

struct TransformDef {
    std::optional<glm::vec3> Position;
    std::optional<glm::vec3> Orientation;
    std::optional<glm::vec3> Scale;

    [[nodiscard]] Transform3D ToTransform3D() const
    {
        Transform3D transform{};
        if (Position) {
            transform.Position = *Position;
        }
        if (Orientation) {
            transform.Orientation = glm::quat(glm::radians(*Orientation));
        }
        if (Scale) {
            transform.Scale = *Scale;
        }
        return transform;
    }
};

struct ModelDef {
    SceneObject Object;
    Animator    Animation;
};

struct TowerDef {
    static constexpr auto kColliderParams = ColliderParams{
        .CollisionCategoryBits = Category::Tower,
        .CollideWithMaskBits   = CollidesWith(Category::Tower, Category::Base),
        .IsTrigger             = false,
        .Offset                = Transform3D{},
        .ShapeParams           = BoxShapeParams{.HalfExtents = glm::vec3(0.35f, 0.65f, 0.35f)},
    };
    static constexpr auto kRigidBodyParams = RigidBodyParams{
        .Type           = rp3d::BodyType::STATIC,
        .Velocity       = 0.0f,
        .Direction      = glm::vec3(0.0f),
        .GravityEnabled = false,
    };

    TowerType    Type{};
    int          Cost{};
    float        Health{};
    TowerAttack  Attack{};
    TransformDef Transform{};
    ModelDef     Model{};
};

struct CreepDef {
    static constexpr auto kColliderParams = ColliderParams{
        .IsTrigger = false,
        .ShapeParams =
            CapsuleShapeParams{
                .Radius = 0.1f,
                .Height = 0.05f,
            },
    };
    static constexpr auto kRigidBodyParams = RigidBodyParams{
        .Type           = rp3d::BodyType::KINEMATIC,
        .Direction      = glm::vec3(0.0f),
        .GravityEnabled = false,
    };

    int          Cost{};
    float        Health{};
    float        Speed{};
    float        Damage{};
    TransformDef Transform{};
    ModelDef     Model{};
};

struct ProjectileDef {
    TransformDef Transform{};
    ModelDef     Model{};
};

struct GameplayDef {
    std::map<TowerType, TowerDef>        Towers;
    std::map<CreepType, CreepDef>        Creeps;
    std::map<std::string, ProjectileDef> Projectiles;
};

template <>
struct glz::meta<ModelDef> {
    using T = ModelDef;

    // NOLINTBEGIN(readability-identifier-naming)
    static constexpr auto ReadName  = [](T& self, const std::string& n) { self.Object.SetName(n); };
    static constexpr auto WriteName = [](auto& self) { return self.Object.WriteName(); };
    static constexpr auto value =
        glz::object("Name", glz::custom<ReadName, WriteName>, "Animation", [](auto& self) -> auto& {
            return self.Animation.AnimationName;
        });
    // NOLINTEND(readability-identifier-naming)
};
