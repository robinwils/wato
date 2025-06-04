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
#include "systems/animation.hpp"
#include "systems/input.hpp"
#include "systems/render.hpp"
#include "systems/sync.hpp"

class GameClient : public Application
{
   public:
    explicit GameClient(int aWidth, int aHeight, char** aArgv)
        : Application(aArgv), mPhysicsEventHandler(&mRegistry)
    {
        mRegistry.ctx().emplace<ActionContextStack>();
        mRegistry.ctx().emplace<WatoWindow>(aWidth, aHeight);
        mRegistry.ctx().emplace<Renderer>();
        mRegistry.ctx().emplace<ENetClient>();
    }
    virtual ~GameClient() = default;

    GameClient(const GameClient&)            = delete;
    GameClient(GameClient&&)                 = delete;
    GameClient& operator=(const GameClient&) = delete;
    GameClient& operator=(GameClient&&)      = delete;

    void Init() override;
    int  Run() override;

   private:
    void networkThread();
    void consumeNetworkResponses();
    void spawnPlayerAndCamera();

    Registry        mRegistry;
    entt::organizer mFrameTimeOrganizer;
    tf::Taskflow    mTaskflow;

    // systems
    InputSystem               mInputSystem;
    RealTimeActionSystem      mRTActionSystem;
    DeterministicActionSystem mFTActionSystem;
    AnimationSystem           mAnimationSystem;
    RenderSystem              mRenderSystem;
    RenderImguiSystem         mRenderImguiSystem;
    CameraSystem              mCameraSystem;
    NetworkSyncSystem         mNetworkSyncSystem;
#if WATO_DEBUG
    PhysicsDebugSystem mPhysicsDbgSystem;
#endif
    EventHandler mPhysicsEventHandler;

    std::optional<clock_type::time_point> mDiscTimerStart;
};
