#pragma once

#include <bx/spscqueue.h>

#include <string>
#include <taskflow/taskflow.hpp>
#include <unordered_map>

#include "core/app/app.hpp"
#include "core/net/enet_server.hpp"
#include "core/net/pocketbase.hpp"
#include "core/types.hpp"

class GameServer : public Application
{
   public:
    explicit GameServer(char** aArgv)
        : Application("server", aArgv),
          mPBClient(mOptions.BackendAddr(), mLogger, ""),
          mServer(mOptions.ServerAddr, mLogger, mPBClient)
    {
    }
    explicit GameServer(const Options& aOptions, const std::string& aAdminToken)
        : Application("server", aOptions),
          mPBClient(mOptions.BackendAddr(), mLogger, aAdminToken),
          mServer(mOptions.ServerAddr, mLogger, mPBClient)
    {
    }
    virtual ~GameServer();

    GameServer(const GameServer&)            = delete;
    GameServer(GameServer&&)                 = delete;
    GameServer& operator=(const GameServer&) = delete;
    GameServer& operator=(GameServer&&)      = delete;

    void Init() override;
    int  Run(tf::Executor& aExecutor) override;
    void ConsumeNetworkRequests();
    void Stop();

    static GameInstanceID GenerateGameInstanceID()
    {
        static std::atomic_int32_t counter{0};
        std::int64_t timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
        return (static_cast<std::uint64_t>(timestamp) << 32)
               | static_cast<std::uint64_t>(counter++);
    }

   protected:
    virtual void OnGameInstanceCreated(Registry& aRegistry) override;

   private:
    void           spawnPlayers(Registry& aRegistry);
    void           createGameInstance(GameInstanceID aGameID);
    GameInstanceID createGameInstance();
    tf::Taskflow   mNetTaskflow;

    PocketBaseClient                             mPBClient;
    ENetServer                                   mServer;
    std::unordered_map<GameInstanceID, Registry> mGameInstances;

    Channel<PBSSE<GameRecord>> mPBGameChan;
};
