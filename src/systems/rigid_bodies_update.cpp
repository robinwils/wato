#include "systems/rigid_bodies_update.hpp"

#include <bgfx/bgfx.h>
#include <bx/bx.h>
#include <spdlog/spdlog.h>

#include "components/rigid_body.hpp"
#include "components/transform3d.hpp"

using namespace entt::literals;

void RigidBodiesUpdateSystem::operator()(Registry& aRegistry)
{
    auto& physics = aRegistry.ctx().get<Physics>();

    auto& rbStorage = aRegistry.storage<entt::reactive>("rigid_bodies_observer"_hs);
    for (auto& e : rbStorage) {
        auto& rb = aRegistry.get<RigidBody>(e);
        auto& c  = aRegistry.get<Collider>(e);
        auto& t  = aRegistry.get<Transform3D>(e);

        if (!rb.Body) {
            spdlog::debug("rigid body creation for {}", e);
            rb.Body  = physics.CreateRigidBody(rb.Params, t);
            c.Handle = physics.AddCollider(rb.Body, c.Params);
        } else {
            spdlog::trace("rigid body update for {}:", e);
        }

        if (rb.Params.Data != rb.Body->getUserData()) {
            spdlog::trace("   user data");
            rb.Body->setUserData(rb.Params.Data);
        }

        if (rb.Params.Type != rb.Body->getType()) {
            spdlog::trace("  body type");
            rb.Body->setType(rb.Params.Type);
        }

        if (rb.Params.Type == reactphysics3d::BodyType::KINEMATIC) {
            spdlog::trace("  linear velocity: {} * {}", rb.Params.Direction, rb.Params.Velocity);
            rb.Body->setLinearVelocity(ToRP3D(rb.Params.Direction * rb.Params.Velocity));
        }

        if (c.Params.IsTrigger != c.Handle->getIsTrigger()) {
            spdlog::trace("  is trigger {}", c.Params.IsTrigger);
            c.Handle->setIsTrigger(c.Params.IsTrigger);
            if (!c.Params.IsTrigger) {
                c.Handle->setIsSimulationCollider(true);
            }
        }

        if (c.Params.CollisionCategoryBits != c.Handle->getCollisionCategoryBits()
            || c.Params.CollideWithMaskBits != c.Handle->getCollideWithMaskBits()) {
            rb.Body->removeCollider(c.Handle);
            c.Handle = physics.AddCollider(rb.Body, c.Params);
        }
    }
}
