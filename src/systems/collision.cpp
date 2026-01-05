#include "systems/collision.hpp"

#include <tuple>
#include <unordered_set>

#include "components/creep.hpp"
#include "components/game.hpp"
#include "components/health.hpp"
#include "components/projectile.hpp"
#include "core/net/enet_server.hpp"
#include "core/physics/physics.hpp"
#include "core/physics/physics_event_listener.hpp"
#include "core/sys/log.hpp"

void CollisionSystem::Execute(Registry& aRegistry, [[maybe_unused]] std::uint32_t aTick)
{
    auto* listener = aRegistry.ctx().find<PhysicsEventListener>();
    if (!listener) {
        return;
    }

    auto& colliderMap = aRegistry.ctx().get<ColliderEntityMap>();

    // Track projectiles to destroy after processing all collisions
    std::unordered_set<entt::entity> projectilesToDestroy;

    for (const auto& event : listener->GetEvents()) {
        if (event.Event != rp3d::OverlapCallback::OverlapPair::EventType::OverlapStart) {
            continue;
        }

        if (!colliderMap.contains(event.Collider1) || !colliderMap.contains(event.Collider2)) {
            continue;
        }

        rp3d::Collider* projCollider    = nullptr;
        rp3d::Collider* targetCollider  = nullptr;
        rp3d::Collider* terrainCollider = nullptr;

        std::tie(projCollider, targetCollider) = MatchColliderPair(
            event.Collider1,
            event.Collider2,
            Category::Projectiles,
            Category::Entities);

        if (!projCollider) {
            std::tie(projCollider, terrainCollider) = MatchColliderPair(
                event.Collider1,
                event.Collider2,
                Category::Projectiles,
                Category::Terrain);

            if (!projCollider) {
                continue;
            }
        }

        entt::entity projectileEntity = colliderMap.at(projCollider);
        if (!aRegistry.valid(projectileEntity)) {
            continue;
        }

        if (targetCollider) {
            entt::entity targetEntity = colliderMap.at(targetCollider);

            WATO_DBG(aRegistry, "projectile {} hits target {}", projectileEntity, targetEntity);
            auto* projectile = aRegistry.try_get<Projectile>(projectileEntity);
            if (!projectile) {
                continue;
            }

            if (aRegistry.valid(targetEntity)) {
                auto* health = aRegistry.try_get<Health>(targetEntity);
                auto* creep  = aRegistry.try_get<Creep>(targetEntity);

                if (health && creep) {
                    aRegistry.patch<Health>(targetEntity, [&projectile](Health& aHealth) {
                        aHealth.Health -= projectile->Damage;
                    });
                    WATO_INFO(
                        aRegistry,
                        "projectile {} hit creep {}, health now {}",
                        projectileEntity,
                        targetEntity,
                        health->Health);

                    if (auto* server = aRegistry.ctx().find<ENetServer>()) {
                        server->EnqueueResponse(new NetworkResponse{
                            .Type     = PacketType::Ack,
                            .PlayerID = 0,
                            .Tick     = aRegistry.ctx().get<GameInstance&>().Tick,
                            .Payload =
                                HealthUpdateResponse{
                                    .Entity = targetEntity,
                                    .Health = health->Health,
                                },
                        });
                    }
                }
            }
            WATO_INFO(
                aRegistry,
                "marking projectile {} for destruction (target hit)",
                projectileEntity);
            projectilesToDestroy.insert(projectileEntity);
        } else if (terrainCollider) {
            // Projectile hit terrain - only destroy if target is invalid or else projectile
            // can get destroy without damaging creep
            auto* projectile = aRegistry.try_get<Projectile>(projectileEntity);
            if (projectile && !aRegistry.valid(projectile->Target)) {
                WATO_INFO(
                    aRegistry,
                    "marking projectile {} for destruction (terrain, target invalid)",
                    projectileEntity);
                projectilesToDestroy.insert(projectileEntity);
            } else {
                WATO_DBG(
                    aRegistry,
                    "projectile {} hit terrain but target still valid, ignoring",
                    projectileEntity);
            }
        }
    }

    for (entt::entity projectile : projectilesToDestroy) {
        if (aRegistry.valid(projectile)) {
            aRegistry.destroy(projectile);
        }
    }

    listener->ClearEvents();
}
