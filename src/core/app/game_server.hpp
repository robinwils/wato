#pragma once

#include <bx/spscqueue.h>

#include <string>
#include <taskflow/taskflow.hpp>
#include <unordered_map>

#include "core/app/app.hpp"
#include "core/net/enet_server.hpp"
#include "core/types.hpp"
#include "systems/action.hpp"

class GameServer : public Application
{
   public:
    explicit GameServer(char** aArgv) : Application(aArgv) {}
    virtual ~GameServer() = default;

    GameServer(const GameServer&)            = delete;
    GameServer(GameServer&&)                 = delete;
    GameServer& operator=(const GameServer&) = delete;
    GameServer& operator=(GameServer&&)      = delete;

    void Init() override;
    int  Run() override;
    void ConsumeNetworkRequests();

    static GameInstanceID GenerateGameInstanceID()
    {
        static std::atomic_int32_t counter{0};
        std::int64_t timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
        return (static_cast<std::uint64_t>(timestamp) << 32)
               | static_cast<std::uint64_t>(counter++);
    }

   protected:
    virtual void OnGameInstanceCreated() override {}

   private:
    GameInstanceID createGameInstance(const NewGameRequest& aNewGame);
    tf::Taskflow   mNetTaskflow;
    tf::Executor   mNetExecutor;

    ENetServer                                   mServer;
    std::unordered_map<GameInstanceID, Registry> mGameInstances;

    // systems
    ServerActionSystem mActionSystem;
};
