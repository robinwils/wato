#pragma once

#include "systems/system.hpp"

/**
 * @brief Physics simulation system (fixed timestep)
 *
 * Updates ReactPhysics3D world at deterministic 60 FPS.
 * Must run before UpdateTransformsSytem.
 */
class PhysicsSystem : public FixedSystem
{
   public:
    using FixedSystem::FixedSystem;

   protected:
    void Execute(Registry& aRegistry, std::uint32_t aTick) override;
};

/**
 * @brief Transform interpolation system (frame time)
 *
 * Interpolates transforms between physics ticks for smooth rendering.
 * Receives interpolation factor (0.0 to 1.0).
 * Must run after PhysicsSystem.
 */
class UpdateTransformsSytem : public FrameSystem
{
   public:
    using FrameSystem::FrameSystem;

   protected:
    void Execute(Registry& aRegistry, float aFactor) override;
};

/**
 * @brief Simulation simulation system (fixed timestep)
 *
 * Updates ReactSimulation3D world at deterministic 60 FPS.
 * Must run before UpdateTransformsSytem.
 */
class SimulationSystem : public FrameSystem
{
   public:
    using FrameSystem::FrameSystem;

   protected:
    void Execute(Registry& aRegistry, float aTick) override;
    void UpdateTransforms(Registry& aRegistry, float aFactor);
};
