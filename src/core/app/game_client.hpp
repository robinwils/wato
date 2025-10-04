#pragma once

#include <bgfx/bgfx.h>

#include <entt/entity/fwd.hpp>
#include <entt/entity/organizer.hpp>
#include <taskflow/taskflow.hpp>

#include "core/app/app.hpp"
#include "core/event_handler.hpp"
#include "core/net/enet_client.hpp"
#include "input/action.hpp"
#include "renderer/renderer.hpp"
#include "resource/cache.hpp"
#include "systems/action.hpp"
#include "systems/animation.hpp"
#include "systems/input.hpp"
#include "systems/render.hpp"
#include "systems/sync.hpp"
#include "systems/tower_built.hpp"

class GameClient : public Application
{
   public:
    explicit GameClient(int aWidth, int aHeight, char** aArgv)
        : Application(aArgv), mPhysicsEventHandler(&mRegistry)
    {
        mRegistry.ctx().emplace<ActionContextStack>();
        mRegistry.ctx().emplace<WatoWindow>(aWidth, aHeight);
        mRegistry.ctx().emplace<Renderer>(mOptions.Renderer());
        mRegistry.ctx().emplace<ENetClient>();
        mRegistry.ctx().emplace<TextureCache>();
        mRegistry.ctx().emplace<ShaderCache>();
        mRegistry.ctx().emplace<ModelCache>();
    }
    GameClient(const GameClient&)            = delete;
    GameClient(GameClient&&)                 = delete;
    GameClient& operator=(const GameClient&) = delete;
    GameClient& operator=(GameClient&&)      = delete;

    virtual ~GameClient()
    {
        std::vector<entt::id_type> ids;
        TRACE("Destroying GameClient");

        for (auto [id, res] : mRegistry.ctx().get<ModelCache>()) {
            ids.push_back(id);
        }
        TRACE("Destroying {} models", ids.size());
        for (auto id : ids) {
            mRegistry.ctx().get<ModelCache>().erase(id);
        }

        ids.clear();
        for (auto [id, res] : mRegistry.ctx().get<ShaderCache>()) {
            ids.push_back(id);
        }
        TRACE("Destroying {} shaders", ids.size());
        for (auto id : ids) {
            mRegistry.ctx().get<ShaderCache>().erase(id);
        }

        ids.clear();
        for (auto [id, res] : mRegistry.ctx().get<TextureCache>()) {
            // TODO: create Texture class in renderer
            bgfx::destroy(res);
            ids.push_back(id);
        }
        TRACE("Destroying {} textures", ids.size());
        for (auto id : ids) {
            mRegistry.ctx().get<TextureCache>().erase(id);
        }

        mRegistry.ctx().erase<ModelCache>();
        mRegistry.ctx().erase<ShaderCache>();
        mRegistry.ctx().erase<TextureCache>();
        mRegistry.ctx().erase<ENetClient>();
        mRegistry.ctx().erase<Renderer>();
        mRegistry.ctx().erase<WatoWindow>();
        mRegistry.ctx().erase<ActionContextStack>();
        mRegistry.clear();
    }

    void Init() override;
    int  Run() override;

   protected:
    virtual void OnGameInstanceCreated() override;

   private:
    void networkThread();
    void consumeNetworkResponses();
    void spawnPlayerAndCamera();
    void prepareGridPreview();
    void setupObservers();

    Registry        mRegistry;
    entt::organizer mFrameTimeOrganizer;
    tf::Taskflow    mTaskflow;
    tf::Taskflow    mNetTaskflow;
    tf::Executor    mNetExecutor;

    // systems
    InputSystem               mInputSystem;
    RealTimeActionSystem      mRTActionSystem;
    DeterministicActionSystem mFTActionSystem;
    AnimationSystem           mAnimationSystem;
    RenderSystem              mRenderSystem;
    RenderImguiSystem         mRenderImguiSystem;
    CameraSystem              mCameraSystem;
    NetworkSyncSystem         mNetworkSyncSystem;
    TowerBuiltSystem          mTowerBuiltSystem;
#if WATO_DEBUG
    PhysicsDebugSystem mPhysicsDbgSystem;
#endif
    EventHandler mPhysicsEventHandler;

    std::optional<clock_type::time_point> mDiscTimerStart;
};
