#include "systems/rigid_bodies_update.hpp"

#include <bgfx/bgfx.h>
#include <bx/bx.h>
#include <spdlog/spdlog.h>

#include <stdexcept>

#include "components/health.hpp"
#include "components/imgui.hpp"
#include "components/placement_mode.hpp"
#include "components/rigid_body.hpp"
#include "components/spawner.hpp"
#include "components/transform3d.hpp"
#include "core/graph.hpp"
#include "core/snapshot.hpp"
#include "core/state.hpp"
#include "resource/cache.hpp"

using namespace entt::literals;

void RigidBodiesUpdateSystem::operator()(Registry& aRegistry)
{
    auto& physics          = aRegistry.ctx().get<Physics>();
    auto& rbStorage        = aRegistry.storage<entt::reactive>("rigid_bodies_observer"_hs);
    auto& placementStorage = aRegistry.storage<entt::reactive>("placement_mode_observer"_hs);

    auto placementMode = aRegistry.view<PlacementMode>();

    for (auto& e : placementStorage) {
        auto& rb = aRegistry.get<RigidBody>(e);
        auto& t  = aRegistry.get<Transform3D>(e);

        if (placementMode->contains(e)) {
            rb.Body->setTransform(t.ToRP3D());
        }
    }

    for (auto& e : rbStorage) {
        auto& rb = aRegistry.get<RigidBody>(e);
        auto& c  = aRegistry.get<Collider>(e);
        auto& t  = aRegistry.get<Transform3D>(e);

        if (!rb.Body) {
            spdlog::debug("rigid body creation for {}", e);
            rb.Body  = physics.CreateRigidBody(rb.Params, t);
            c.Handle = physics.AddCollider(rb.Body, c.Params);
        } else {
            spdlog::debug("rigid body update for {}:", e);
        }

        if (rb.Params.Data != rb.Body->getUserData()) {
            spdlog::debug("   user data");
            rb.Body->setUserData(rb.Params.Data);
        }

        if (rb.Params.Type != rb.Body->getType()) {
            spdlog::debug("  body type");
            rb.Body->setType(rb.Params.Type);
        }

        if (rb.Params.Type == reactphysics3d::BodyType::KINEMATIC) {
            spdlog::debug("  linear velocity: {} * {}", rb.Params.Direction, rb.Params.Velocity);
            rb.Body->setLinearVelocity(ToRP3D(rb.Params.Direction * rb.Params.Velocity));
        }

        if (c.Params.IsTrigger != c.Handle->getIsTrigger()) {
            spdlog::debug("  is trigger {}", c.Params.IsTrigger);
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

    auto snapshotView = entt::basic_view{rbStorage, placementStorage};

    GameState&        state = aRegistry.ctx().get<GameStateBuffer&>().Latest();
    ByteOutputArchive outAr(state.Snapshot);

    entt::snapshot{aRegistry}
        .get<Transform3D>(outAr, rbStorage.begin(), rbStorage.end())
        .get<RigidBody>(outAr, rbStorage.begin(), rbStorage.end())
        .get<Collider>(outAr, rbStorage.begin(), rbStorage.end());
}
