#pragma once

#include <bx/spscqueue.h>

#include "core/app/app.hpp"
#include "core/net/enet_server.hpp"

class GameServer : public Application
{
   public:
    explicit GameServer(char** aArgv) : Application(0, 0, aArgv)
    {
        mRegistry.ctx().emplace<ActionBuffer>();
    }
    virtual ~GameServer() = default;

    GameServer(const GameServer&)            = delete;
    GameServer(GameServer&&)                 = delete;
    GameServer& operator=(const GameServer&) = delete;
    GameServer& operator=(GameServer&&)      = delete;

    void Init();
    int  Run();
    void ConsumeNetworkEvents();

   private:
    ENetServer mServer;
};
