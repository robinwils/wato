#pragma once

#include <entt/entity/organizer.hpp>

#include "registry/registry.hpp"
#include "systems/system.hpp"

/**
 * @brief Main rendering system (frame time)
 *
 * Renders all scene objects, handles instancing, lighting, and grid visualization.
 * Runs at variable frame rate.
 */
class RenderSystem : public FrameSystem
{
   public:
    using FrameSystem::FrameSystem;

   protected:
    void Execute(Registry& aRegistry, float aDelta) override;

    const char* Name() const override { return "RenderSystem"; }

   private:
    void updateGridTexture(Registry& aRegistry);
    void renderGrid(Registry& aRegistry);
};

/**
 * @brief ImGui rendering system (frame time)
 *
 * Renders debug UI and overlays.
 * Runs at variable frame rate.
 */
class RenderImguiSystem : public FrameSystem
{
   public:
    using FrameSystem::FrameSystem;

    const char* Name() const override { return "RenderImguiSystem"; }

   protected:
    void Execute(Registry& aRegistry, float aDelta) override;
};

/**
 * @brief Camera system (frame time)
 *
 * Updates camera view/projection matrices.
 * Runs at variable frame rate.
 */
class CameraSystem : public FrameSystem
{
   public:
    using FrameSystem::FrameSystem;

    const char* Name() const override { return "CameraSystem"; }

   protected:
    void Execute(Registry& aRegistry, float aDelta) override;
};

#if WATO_DEBUG
/**
 * @brief Physics debug rendering system (frame time)
 *
 * Renders physics debug visualization (colliders, constraints).
 * Runs at variable frame rate.
 */
class PhysicsDebugSystem : public FrameSystem
{
   public:
    using FrameSystem::FrameSystem;

    const char* Name() const override { return "PhysicsDebugSystem"; }

   protected:
    void Execute(Registry& aRegistry, float aDelta) override;
};
#endif
