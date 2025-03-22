#pragma once

#include "core/window.hpp"
#include "systems/system.hpp"

class PlayerInputSystem : public System<PlayerInputSystem>
{
   public:
    void operator()(Registry& aRegistry, const float aDeltaTime, WatoWindow& aWin);

    static constexpr const char* StaticName() { return "PlayerInputSystem"; }

   private:
    void towerPlacementMode(Registry& aRegistry, WatoWindow& aWin, bool aEnable);
    void buildTower(Registry& aRegistry);
    void cameraInput(Registry& aRegistry, const float aDeltaTime);

    glm::vec3 getMouseRay(Registry& aRegistry, WatoWindow& aWin) const;
};
