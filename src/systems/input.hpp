#pragma once

#include "core/window.hpp"
#include "systems/system.hpp"

class InputSystem : public System<InputSystem>
{
   public:
    void operator()(Registry& aRegistry);

    static constexpr const char* StaticName() { return "InputSystem"; }

   private:
    void handleMouseMovement(Registry& aRegistry);
};
