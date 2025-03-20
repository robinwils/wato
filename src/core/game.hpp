#pragma once

#include <memory>

#include "bgfx/bgfx.h"
#include "core/event_handler.hpp"
#include "core/registry.hpp"
#include "core/window.hpp"
#include "systems/systems.hpp"

class Game
{
   public:
    explicit Game(int aWidth, int aHeight)
        : mWindow(std::make_unique<WatoWindow>(aWidth, aHeight)),
          mActionSystem(&mRegistry, aWidth, aHeight),
          mPhysicsEventHandler(&mRegistry, &mActionSystem)
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
    ActionSystem mActionSystem;

    // Physics event handler
    EventHandler mPhysicsEventHandler;
};
