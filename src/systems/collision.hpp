#pragma once

#include <unordered_set>

#include "core/physics/physics_event_listener.hpp"
#include "systems/system.hpp"

/**
 * @brief Collision response system (fixed timestep)
 *
 * Processes trigger events from PhysicsEventListener and applies game logic
 *
 */
class CollisionSystem : public FixedSystem
{
   public:
    using FixedSystem::FixedSystem;

   protected:
    void Execute(Registry& aRegistry, std::uint32_t aTick) override;

   private:
    void projectileHits(
        Registry&                         aRegistry,
        const TriggerEvent&               aEvent,
        std::unordered_set<entt::entity>& aToDestroy);
    void creepHitsBase(Registry& aRegistry, const TriggerEvent& aEvent);
};
