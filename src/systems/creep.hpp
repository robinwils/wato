#pragma once

#include "systems/system.hpp"

class CreepSystem : public System<CreepSystem>
{
   public:
    void operator()(Registry& aRegistry, const float aDeltaTime);

    static constexpr const char* StaticName() { return "CreepSystem"; }
};
