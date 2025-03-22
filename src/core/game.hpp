#pragma once

#include <memory>

#include "core/event_handler.hpp"
#include "core/registry.hpp"
#include "core/window.hpp"
#include "renderer/renderer.hpp"
#include "systems/input.hpp"
#include "systems/physics.hpp"
#include "systems/render.hpp"

class Game
{
   public:
    explicit Game(int aWidth, int aHeight)
        : mWindow(aWidth, aHeight), mPhysicsEventHandler(&mRegistry)
    {
    }
    virtual ~Game() = default;

    Game(const Game &)            = delete;
    Game(Game &&)                 = delete;
    Game &operator=(const Game &) = delete;
    Game &operator=(Game &&)      = delete;

    void Init();
    int  Run();

   private:
    WatoWindow mWindow;

    Renderer mRenderer;

    Registry mRegistry;

    // TODO: I don't know where to put this yet, maybe handle this better ?
    PhysicsSystem mPhysicsSystem;
#if WATO_DEBUG
    PhysicsDebugSystem mPhysicsDbgSystem;
#endif
    PlayerInputSystem mPlayerInputSystem;
    RenderSystem      mRenderSystem;
    RenderImguiSystem mRenderImguiSystem;
    CameraSystem      mCameraSystem;

    SystemRegistry mSystems;

    // Physics event handler
    EventHandler mPhysicsEventHandler;
};
