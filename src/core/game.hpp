#pragma once

#include <memory>

#include "bgfx/bgfx.h"
#include "core/event_handler.hpp"
#include "core/registry.hpp"
#include "core/window.hpp"
#include "entt/signal/fwd.hpp"
#include "systems/input.hpp"
#include "systems/physics.hpp"
#include "systems/render.hpp"

class Game
{
   public:
    explicit Game(int aWidth, int aHeight)
        : mWindow(std::make_unique<WatoWindow>(aWidth, aHeight)), mPhysicsEventHandler(&mRegistry)
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
    const bgfx::ViewId CLEAR_VIEW = 0;

    std::unique_ptr<WatoWindow> mWindow;

    bgfx::Init mInitParams;

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
