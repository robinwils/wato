#pragma once

#include <reactphysics3d/reactphysics3d.h>

#include <glm/ext/vector_float3.hpp>

#include "core/physics/physics.hpp"
#include "registry/registry.hpp"

struct RigidBody {
    RigidBodyParams  Params;
    rp3d::RigidBody* Body{nullptr};
    static void      on_destroy(entt::registry& aRegistry, const entt::entity aEntity)
    {
        const auto& body = aRegistry.get<RigidBody>(aEntity);
        if (body.Body) {
            aRegistry.ctx().get<Physics>().World()->destroyRigidBody(body.Body);
        }
    }
};

struct Collider {
    ColliderParams  Params;
    rp3d::Collider* Handle{nullptr};
};
