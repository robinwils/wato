#pragma once

#include <entt/entity/fwd.hpp>
#include <entt/entity/organizer.hpp>
#include <taskflow/taskflow.hpp>

#include "core/app/app.hpp"
#include "core/event_handler.hpp"
#include "core/net/enet_client.hpp"
#include "input/action.hpp"
#include "renderer/renderer.hpp"
#include "systems/action.hpp"
#include "systems/input.hpp"
#include "systems/render.hpp"

class Game : public Application
{
   public:
    explicit Game(int aWidth, int aHeight, char** aArgv)
        : Application(aWidth, aHeight, aArgv), mPhysicsEventHandler(&mRegistry)
    {
        mRegistry.ctx().emplace<ActionBuffer>();
        mRegistry.ctx().emplace<ActionContextStack>();
        mRegistry.ctx().emplace<WatoWindow>(aWidth, aHeight);
        mRegistry.ctx().emplace<Renderer>();
        mRegistry.ctx().emplace<ENetClient>();
    }
    virtual ~Game() = default;

    Game(const Game&)            = delete;
    Game(Game&&)                 = delete;
    Game& operator=(const Game&) = delete;
    Game& operator=(Game&&)      = delete;

    void Init() override;
    int  Run() override;

   private:
    entt::organizer           mFrameTimeOrganizer;
    tf::Taskflow              mTaskflow;
    InputSystem               mInputSystem;
    DeterministicActionSystem mFTActionSystem;
    RealTimeActionSystem      mActionSystem;
    RenderSystem              mRenderSystem;
    RenderImguiSystem         mRenderImguiSystem;
    CameraSystem              mCameraSystem;
#if WATO_DEBUG
    PhysicsDebugSystem mPhysicsDbgSystem;
#endif
    EventHandler mPhysicsEventHandler;
};
