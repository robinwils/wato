#pragma once

#include "core/app/app.hpp"
#include "systems/input.hpp"
#include "systems/render.hpp"

class Game : public Application
{
   public:
    explicit Game(int aWidth, int aHeight)
        : Application(aWidth, aHeight), mPhysicsEventHandler(&mRegistry)
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
    PlayerInputSystem mPlayerInputSystem;
    RenderSystem      mRenderSystem;
    RenderImguiSystem mRenderImguiSystem;
    CameraSystem      mCameraSystem;
#if WATO_DEBUG
    PhysicsDebugSystem mPhysicsDbgSystem;
#endif
    EventHandler mPhysicsEventHandler;
};
