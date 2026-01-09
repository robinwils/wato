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

   protected:
    void Execute(Registry& aRegistry, float aDelta) override;

   private:
    void handleMouseMovement(Registry& aRegistry);
};
