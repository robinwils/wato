#include "systems/collision.hpp"

#include <tuple>
#include <unordered_set>

#include "components/creep.hpp"
#include "components/game.hpp"
#include "components/health.hpp"
#include "components/projectile.hpp"
#include "components/spawner.hpp"
#include "core/net/enet_server.hpp"
#include "core/physics/physics.hpp"
#include "core/physics/physics_event_listener.hpp"
#include "core/sys/log.hpp"
#include "registry/registry.hpp"

void CollisionSystem::Execute(Registry& aRegistry, [[maybe_unused]] std::uint32_t aTick)
{
    auto* listener = aRegistry.ctx().find<PhysicsEventListener>();
    if (!listener) {
        return;
    }

    const auto& colliderMap = GetSingletonComponent<ColliderEntityMap>(aRegistry);

    // Track projectiles to destroy after processing all collisions
    std::unordered_set<entt::entity> entitiesToDestroy;

    for (const auto& event : listener->GetEvents()) {
        if (event.Event != rp3d::OverlapCallback::OverlapPair::EventType::OverlapStart) {
            continue;
        }

        if (!colliderMap.contains(event.Collider1) || !colliderMap.contains(event.Collider2)) {
            continue;
        }

        projectileHits(aRegistry, event, entitiesToDestroy);
        creepHitsPlayerBase(aRegistry, event);
    }

    for (entt::entity e : entitiesToDestroy) {
        if (aRegistry.valid(e)) {
            aRegistry.destroy(e);
        }
    }

    listener->ClearEvents();
}

void CollisionSystem::projectileHits(
    Registry&                         aRegistry,
    const TriggerEvent&               aEvent,
    std::unordered_set<entt::entity>& aToDestroy)
{
    const auto&           colliderMap     = GetSingletonComponent<ColliderEntityMap>(aRegistry);
    const rp3d::Collider* projCollider    = nullptr;
    const rp3d::Collider* targetCollider  = nullptr;
    const rp3d::Collider* terrainCollider = nullptr;

    std::tie(projCollider, targetCollider) = aEvent.CreepCollision(Category::Projectiles);

    if (!projCollider) {
        std::tie(projCollider, terrainCollider) =
            aEvent.Matches(Category::Projectiles, Category::Terrain);

        if (!projCollider) {
            return;
        }
    }

    entt::entity projectileEntity = colliderMap.at(projCollider);
    if (!aRegistry.valid(projectileEntity) || aToDestroy.contains(projectileEntity)) {
        return;
    }

    if (targetCollider) {
        entt::entity targetEntity = colliderMap.at(targetCollider);

        WATO_DBG(aRegistry, "projectile {} hits target {}", projectileEntity, targetEntity);
        auto* projectile = aRegistry.try_get<Projectile>(projectileEntity);
        if (!projectile) {
            return;
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
                    server->BroadcastResponse(
                        GetPlayerIDs(aRegistry),
                        PacketType::Ack,
                        GetSingletonComponent<GameInstance&>(aRegistry).Tick,

                        HealthUpdateResponse{
                            .Entity = targetEntity,
                            .Health = health->Health,
                        });
                }
            }
        }
        WATO_INFO(
            aRegistry,
            "marking projectile {} for destruction (target hit)",
            projectileEntity);
        aToDestroy.insert(projectileEntity);
    } else if (terrainCollider) {
        // Projectile hit terrain - only destroy if target is invalid or else projectile
        // can get destroy without damaging creep
        auto* projectile = aRegistry.try_get<Projectile>(projectileEntity);
        if (projectile && !aRegistry.valid(projectile->Target)) {
            WATO_INFO(
                aRegistry,
                "marking projectile {} for destruction (terrain, target invalid)",
                projectileEntity);
            aToDestroy.insert(projectileEntity);
        } else {
            WATO_DBG(
                aRegistry,
                "projectile {} hit terrain but target still valid, ignoring",
                projectileEntity);
        }
    }
}

void CollisionSystem::creepHitsPlayerBase(Registry& aRegistry, const TriggerEvent& aEvent)
{
    const auto& colliderMap = GetSingletonComponent<ColliderEntityMap>(aRegistry);

    auto [playerCollider, creepCollider] = aEvent.CreepCollision(Category::Base);

    if (creepCollider && playerCollider) {
        entt::entity creepEntity  = colliderMap.at(creepCollider);
        entt::entity playerEntity = colliderMap.at(playerCollider);

        if (!aRegistry.valid(playerEntity) || !aRegistry.valid(creepEntity)) {
            return;
        }

        auto& creep  = aRegistry.get<Creep>(creepEntity);
        auto& health = aRegistry.patch<Health>(playerEntity, [&creep](Health& aHealth) {
            aHealth.Health -= creep.Damage;
        });

        WATO_DBG(
            aRegistry,
            "creep hits base, player {} looses {} health, {} left",
            playerEntity,
            creep.Damage,
            health.Health);

        aRegistry.patch<Health>(creepEntity, [](Health& aHealth) { aHealth.Health = 0.0f; });
        if (auto* server = aRegistry.ctx().find<ENetServer>()) {
            server->BroadcastResponse(
                GetPlayerIDs(aRegistry),
                PacketType::Ack,
                GetSingletonComponent<GameInstance&>(aRegistry).Tick,
                HealthUpdateResponse{
                    .Entity = playerEntity,
                    .Health = health.Health,
                });
        }
    }
}
