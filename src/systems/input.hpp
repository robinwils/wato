#pragma once

#include "systems/system.hpp"

/**
 * @brief Input handling system (frame time)
 *
 * Processes keyboard/mouse input and generates actions.
 * Runs at variable frame rate for responsive input.
 */
class InputSystem : public FrameSystem
{
   public:
    using FrameSystem::FrameSystem;

    const char* Name() const override { return "InputSystem"; }

   protected:
    void Execute(Registry& aRegistry, float aDelta) override;

   private:
    void handleMouseMovement(Registry& aRegistry);
    void handleMouseSelection(Registry& aRegistry, const rp3d::Ray& aMouseRay);
    void createActions(Registry& aRegistry, float aDelta);
};
