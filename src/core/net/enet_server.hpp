#pragma once

#include <enet.h>

#include "core/net/net.hpp"

class ENetServer
{
   public:
    ENetServer()                              = default;
    ENetServer(ENetServer &&)                 = default;
    ENetServer(const ENetServer &)            = delete;
    ENetServer &operator=(ENetServer &&)      = default;
    ENetServer &operator=(const ENetServer &) = delete;
    ~ENetServer();

    void Init();
    void Run();

   private:
    enet_host_ptr mServer;
};
