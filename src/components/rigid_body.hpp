#pragma once

#include <reactphysics3d/reactphysics3d.h>

#include <glm/ext/vector_float3.hpp>

#include "core/physics/physics.hpp"
#include "core/serialize.hpp"
#include "registry/registry.hpp"

struct RigidBody {
    RigidBodyParams  Params;
    rp3d::RigidBody* Body{nullptr};

    static void on_destroy(entt::registry& aRegistry, const entt::entity aEntity)
    {
        const auto& body = aRegistry.get<RigidBody>(aEntity);
        if (body.Body) {
            aRegistry.ctx().get<Physics>().World()->destroyRigidBody(body.Body);
        }
    }

    bool Archive(auto& aArchive)
    {
        if (!ArchiveValue(aArchive, Params.Type, 0, 3)) return false;
        if (!ArchiveValue(aArchive, Params.Velocity, 0.0f, 100.0f)) return false;
        if (!ArchiveVector(aArchive, Params.Direction, 0.0f, 1.0f)) return false;
        if (!ArchiveBool(aArchive, Params.GravityEnabled)) return false;
        return true;
    }
};

struct Collider {
    ColliderParams  Params;
    rp3d::Collider* Handle{nullptr};

    bool Archive(auto& aArchive)
    {
        uint32_t maxCategoryValue = SafeI32(Category::Count);
        if (!ArchiveValue(aArchive, Params.CollisionCategoryBits, 0u, maxCategoryValue))
            return false;
        if (!ArchiveValue(aArchive, Params.CollideWithMaskBits, 0u, maxCategoryValue)) return false;
        if (!ArchiveBool(aArchive, Params.IsTrigger)) return false;
        if (!Params.Offset.Archive(aArchive)) return false;

        return ArchiveVariant(aArchive, Params.ShapeParams);
    }
};
