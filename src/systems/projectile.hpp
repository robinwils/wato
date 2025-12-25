#pragma once

#include "systems/system.hpp"

/**
 * @brief Projectile tracking system (fixed timestep)
 *
 * Updates projectile direction to track targets and destroys projectiles when target is invalid.
 * Runs at deterministic 60 FPS.
 */
class ProjectileSystem : public FixedSystem
{
   public:
    using FixedSystem::FixedSystem;

   protected:
    void Execute(Registry& aRegistry, std::uint32_t aTick) override;
};
