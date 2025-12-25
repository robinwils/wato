#pragma once

#include <bgfx/bgfx.h>

#include <entt/entity/fwd.hpp>

#include "core/app/app.hpp"
#include "core/net/enet_client.hpp"
#include "core/physics/physics_event_listener.hpp"
#include "input/action.hpp"
#include "registry/registry.hpp"
#include "renderer/renderer.hpp"
#include "resource/cache.hpp"

class GameClient : public Application
{
   public:
    explicit GameClient(int aWidth, int aHeight, char** aArgv) : Application("client", aArgv)
    {
        initContext(aWidth, aHeight);
    }

    explicit GameClient(int aWidth, int aHeight, const Options& aOptions)
        : Application("client", aOptions)
    {
        initContext(aWidth, aHeight);
    }

    GameClient(const GameClient&)            = delete;
    GameClient(GameClient&&)                 = delete;
    GameClient& operator=(const GameClient&) = delete;
    GameClient& operator=(GameClient&&)      = delete;

    virtual ~GameClient()
    {
        WATO_TRACE(mRegistry, "Destroying GameClient");

        std::vector<entt::id_type> ids;

        for (auto [id, res] : mRegistry.ctx().get<ModelCache>()) {
            ids.push_back(id);
        }
        WATO_TRACE(mRegistry, "Destroying {} models", ids.size());
        for (auto id : ids) {
            mRegistry.ctx().get<ModelCache>().erase(id);
        }

        ids.clear();
        for (auto [id, res] : mRegistry.ctx().get<ShaderCache>()) {
            ids.push_back(id);
        }
        WATO_TRACE(mRegistry, "Destroying {} shaders", ids.size());
        for (auto id : ids) {
            mRegistry.ctx().get<ShaderCache>().erase(id);
        }

        ids.clear();
        for (auto [id, res] : mRegistry.ctx().get<TextureCache>()) {
            // TODO: create Texture class in renderer
            bgfx::destroy(res);
            ids.push_back(id);
        }
        WATO_TRACE(mRegistry, "Destroying {} textures", ids.size());
        for (auto id : ids) {
            mRegistry.ctx().get<TextureCache>().erase(id);
        }

        mRegistry.ctx().erase<ModelCache>();
        mRegistry.ctx().erase<ShaderCache>();
        mRegistry.ctx().erase<TextureCache>();
        mRegistry.ctx().erase<ENetClient>();
        mRegistry.ctx().erase<BgfxRenderer>();
        mRegistry.ctx().erase<WatoWindow>();
        mRegistry.ctx().erase<ActionContextStack>();
        mRegistry.clear();
    }

    void Init() override;
    int  Run(tf::Executor& aExecutor) override;

   protected:
    virtual void OnGameInstanceCreated(Registry& aRegistry) override;

   private:
    inline void initContext(int aWidth, int aHeight)
    {
        mRegistry.ctx().emplace<Logger&>(mLogger);
        mRegistry.ctx().emplace<ActionContextStack>();
        mRegistry.ctx().emplace<WatoWindow>(aWidth, aHeight);
        mRegistry.ctx().emplace<BgfxRenderer>(mOptions.Renderer());
        mRegistry.ctx().emplace<ENetClient>(mLogger);
        mRegistry.ctx().emplace<TextureCache>();
        mRegistry.ctx().emplace<ShaderCache>();
        mRegistry.ctx().emplace<ModelCache>();
        mRegistry.ctx().emplace<EntitySyncMap>();
    }
    void networkThread();
    void consumeNetworkResponses();
    void spawnPlayerAndCamera();
    void prepareGridPreview();

    Registry mRegistry;

    std::optional<clock_type::time_point> mDiscTimerStart;
};
