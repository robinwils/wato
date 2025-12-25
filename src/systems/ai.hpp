#pragma once

#include "systems/system.hpp"

/**
 * @brief AI pathfinding system (fixed timestep)
 *
 * Updates creep pathfinding and movement direction based on graph paths.
 * Runs at deterministic 60 FPS.
 */
class AiSystem : public FixedSystem
{
   public:
    using FixedSystem::FixedSystem;

   protected:
    void Execute(Registry& aRegistry, std::uint32_t aTick) override;
};
