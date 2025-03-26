#pragma once

#include <bx/spscqueue.h>

#include "core/app/app.hpp"
#include "core/net/enet_server.hpp"

class GameServer : public Application
{
   public:
    explicit GameServer() : Application(0, 0) {}
    virtual ~GameServer() = default;

    GameServer(const GameServer &)            = delete;
    GameServer(GameServer &&)                 = delete;
    GameServer &operator=(const GameServer &) = delete;
    GameServer &operator=(GameServer &&)      = delete;

    void Init();
    int  Run();

   private:
    ENetServer mServer;
};
