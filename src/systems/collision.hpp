#pragma once

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
};
