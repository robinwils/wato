#pragma once

#include <bx/spscqueue.h>

#include <string>
#include <unordered_map>

#include "core/app/app.hpp"
#include "core/net/enet_server.hpp"
#include "core/types.hpp"

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
    void ConsumeNetworkEvents();

   private:
    GameInstanceID createGameInstance(const NewGameRequest& aNewGame);
    void           advanceSimulation(Registry& aRegistry);

    ENetServer                                   mServer;
    std::unordered_map<GameInstanceID, Registry> mGameInstances;
};
