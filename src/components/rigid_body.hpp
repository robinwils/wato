#pragma once

#include <reactphysics3d/reactphysics3d.h>

#include <glm/ext/vector_float3.hpp>

#include "core/physics/physics.hpp"
#include "core/serialize.hpp"
#include "registry/registry.hpp"

struct Collider {
    ColliderParams  Params;
    rp3d::Collider* Handle{nullptr};

    static void on_destroy(entt::registry& aRegistry, const entt::entity aEntity)
    {
        auto& collider = aRegistry.get<Collider>(aEntity);

        // Remove from collider map
        if (collider.Handle) {
            aRegistry.ctx().get<ColliderEntityMap>().erase(collider.Handle);
        }
        collider.Handle = nullptr;
    }

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

struct RigidBody {
    RigidBodyParams  Params;
    rp3d::RigidBody* Body{nullptr};

    static void on_destroy(entt::registry& aRegistry, const entt::entity aEntity)
    {
        auto& body = aRegistry.get<RigidBody>(aEntity);

        // Destroy physics body
        if (body.Body) {
            aRegistry.ctx().get<Physics>().World()->destroyRigidBody(body.Body);
        }
        body.Body = nullptr;
    }

    bool Archive(auto& aArchive) { return Params.Archive(aArchive); }
};
