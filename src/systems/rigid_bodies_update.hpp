#pragma once

#include "systems/system.hpp"

/**
 * @brief Rigid body synchronization system (fixed timestep)
 *
 * Syncs ECS components to ReactPhysics3D bodies and colliders.
 * Runs at deterministic 60 FPS.
 */
class RigidBodiesUpdateSystem : public FixedSystem
{
   public:
    using FixedSystem::FixedSystem;

   protected:
    void Execute(Registry& aRegistry, std::uint32_t aTick) override;
};
