#pragma once

#include "core/window.hpp"
#include "systems/system.hpp"

class PlayerInputSystem : public System<PlayerInputSystem>
{
   public:
    void operator()(Registry& aRegistry, const float aDeltaTime);

    static constexpr const char* StaticName() { return "PlayerInputSystem"; }

   private:
    void towerPlacementMode(Registry& aRegistry, bool aEnable);
    void buildTower(Registry& aRegistry);
    void cameraInput(Registry& aRegistry, const float aDeltaTime);
    void creepSpawn(Registry& aRegistry);

    glm::vec3 getMouseRay(Registry& aRegistry) const;
};
