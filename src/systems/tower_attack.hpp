#pragma once

#include "registry/registry.hpp"
#include "systems/system.hpp"

class TowerAttackSystem : public System<TowerAttackSystem>
{
   public:
    void operator()(Registry& aRegistry, float aDeltaTime);
};
