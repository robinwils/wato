#pragma once

#include "systems/system.hpp"

/**
 * @brief Tower attack system (fixed timestep)
 *
 * Handles tower target acquisition and projectile spawning.
 * Runs at deterministic 60 FPS.
 */
class TowerAttackSystem : public FixedSystem
{
   public:
    using FixedSystem::FixedSystem;

   protected:
    void Execute(Registry& aRegistry, std::uint32_t aTick) override;
};
