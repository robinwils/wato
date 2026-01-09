#pragma once

#include "systems/system.hpp"

/**
 * @brief Health system (fixed timestep)
 *
 * Destroys entities when their health drops to zero or below.
 * Runs at deterministic 60 FPS.
 */
class HealthSystem : public FixedSystem
{
   public:
    using FixedSystem::FixedSystem;

   protected:
    void Execute(Registry& aRegistry, std::uint32_t aTick) override;
};
