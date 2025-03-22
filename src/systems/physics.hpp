#pragma once

#include "config.h"
#include "systems/system.hpp"

class PhysicsSystem : public System<PhysicsSystem>
{
   public:
    void operator()(Registry& aRegistry, const float aDeltaTime);

    static constexpr const char* StaticName() { return "PhysicsSystem"; }
};

#if WATO_DEBUG
class PhysicsDebugSystem : public System<PhysicsDebugSystem>
{
   public:
    void operator()(Registry& aRegistry, const float aDeltaTime);

    static constexpr const char* StaticName() { return "PhysicsDebugSystem "; }
};
#endif
