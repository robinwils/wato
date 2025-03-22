#pragma once
#include <bgfx/bgfx.h>
#include <bimg/decode.h>

#include <entt/entity/registry.hpp>

#include "components/physics.hpp"
#include "core/window.hpp"

class EventHandler;

class Registry : public entt::basic_registry<entt::entity>
{
   public:
    void Init(WatoWindow& aWin, EventHandler* aPhyHandler);
    void SpawnPlane();
    void SpawnMap(uint32_t aWidth, uint32_t aHeight);
    void SpawnLight();
    void LoadShaders();
    void LoadModels();
    void SpawnPlayerAndCamera();

    Input&   GetPlayerInput() { return ctx().get<Input&>(); }
    Physics& GetPhysics() { return ctx().get<Physics&>(); }
};
