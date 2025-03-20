#pragma once
#include <core/registry.hpp>
#include <glm/ext/vector_float3.hpp>

#include "components/physics.hpp"
#include "config.h"
#include "core/action.hpp"
#include "core/window.hpp"
#include "entt/entity/entity.hpp"
#include "entt/entity/fwd.hpp"
#include "input/input.hpp"

void renderSceneObjects(Registry& aRegistry, const float aTimeDelta);
void cameraSystem(Registry& aRegistry, float aWidth, float aHeight);
void processInputs(Registry& aRegistry, const double aTimeDelta);
void renderImgui(Registry& aRegistry, WatoWindow& aWin);
void physicsSystem(Registry& aRegistry, double aDeltaTime);

#if WATO_DEBUG
void physicsDebugRenderSystem(Registry& aRegistry);
#endif

struct ActionSystem {
   public:
    ActionSystem(Registry* aRegistry, int aWidth, int aHeight)
        : mRegistry(aRegistry),
          mWinWidth(static_cast<float>(aWidth)),
          mWinHeight(static_cast<float>(aHeight))
    {
    }
    virtual ~ActionSystem()                      = default;
    ActionSystem(const ActionSystem&)            = default;
    ActionSystem(ActionSystem&&)                 = default;
    ActionSystem& operator=(const ActionSystem&) = default;
    ActionSystem& operator=(ActionSystem&&)      = default;

    void InitListeners(Input& aInput);
    void UdpateWinSize(int aW, int aH);
    void SetCanBuild(bool aCanBuild) { mCanBuild = aCanBuild; }

   private:
    [[nodiscard]] glm::vec3 getMouseRay() const;

    // handlers
    void cameraMovement(CameraMovement);
    void buildTower(BuildTower);
    void towerPlacementMode(TowerPlacementMode);

    Registry*    mRegistry;
    entt::entity mGhostTower{entt::null};
    float        mWinWidth, mWinHeight;
    bool         mCanBuild = true;
};
