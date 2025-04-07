#pragma once

#include "systems/system.hpp"

class PhysicsSystem : public System<PhysicsSystem>
{
   public:
    void operator()(Registry& aRegistry, const float aDeltaTime);

    static constexpr const char* StaticName() { return "PhysicsSystem"; }
};

class UpdateTransformsSytem : public System<UpdateTransformsSytem>
{
   public:
    void operator()(Registry& aRegistry, const float aFactor);

    static constexpr const char* StaticName() { return "UpdateTransformsSystem"; }
};
