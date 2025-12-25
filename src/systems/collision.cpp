#include "systems/collision.hpp"

#include <tuple>

#include "components/creep.hpp"
#include "components/health.hpp"
#include "components/projectile.hpp"
#include "core/physics/physics_event_listener.hpp"
#include "core/physics/physics.hpp"
#include "core/sys/log.hpp"

void CollisionSystem::Execute(Registry& aRegistry, [[maybe_unused]] std::uint32_t aTick)
{
    auto* listener = aRegistry.ctx().find<PhysicsEventListener>();
    if (!listener) {
        return;
    }

    auto& colliderMap = aRegistry.ctx().get<ColliderEntityMap>();

    for (const auto& event : listener->GetEvents()) {
        // Only process collision start events
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

        // Map colliders to entities
        entt::entity projectileEntity = colliderMap.at(projCollider);
        if (!aRegistry.valid(projectileEntity)) {
            continue;
        }

        // Handle target (if it's an entity, not terrain)
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
                    health->Health -= projectile->Damage;
                    WATO_DBG(
                        aRegistry,
                        "projectile {} hit creep {}, health now {}",
                        projectileEntity,
                        targetEntity,
                        health->Health);
                }
            }
        }

        // Destroy projectile regardless of what it hit
        aRegistry.destroy(projectileEntity);
        WATO_TRACE(aRegistry, "destroyed projectile {} on collision", projectileEntity);
    }

    listener->ClearEvents();
}
