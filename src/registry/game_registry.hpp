#pragma once

#include <bimg/decode.h>

#include "core/engine.hpp"
#include "core/physics.hpp"
#include "core/window.hpp"
#include "registry/registry.hpp"

class EventHandler;

class GameRegistry : public Registry
{
   public:
    void LoadResources();
    void SpawnPlane();
    void SpawnMap(uint32_t aWidth, uint32_t aHeight);
    void SpawnLight();
    void LoadShaders();
    void LoadModels();
    void SpawnPlayerAndCamera();

    Input&      GetPlayerInput() { return ctx().get<Engine&>().GetPlayerInput(); }
    Physics&    GetPhysics() { return ctx().get<Engine&>().GetPhysics(); }
    WatoWindow& GetWindow() { return ctx().get<Engine&>().GetWindow(); }
    Renderer&   GetRenderer() { return ctx().get<Engine&>().GetRenderer(); }
};
