#pragma once

#include "registry/registry.hpp"
#include "systems/system.hpp"

class ProjectileSystem : public System<ProjectileSystem>
{
   public:
    void operator()(Registry& aRegistry, float aDeltaTime);
};
