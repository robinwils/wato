#pragma once

#include "systems/system.hpp"

/**
 * @brief Tower construction system (fixed timestep)
 *
 * Handles tower building, obstacle registration, and pathfinding updates.
 * Runs at deterministic 60 FPS.
 */
class TowerBuiltSystem : public FixedSystem
{
   public:
    using FixedSystem::FixedSystem;

   protected:
    void Execute(Registry& aRegistry, std::uint32_t aTick) override;
};
