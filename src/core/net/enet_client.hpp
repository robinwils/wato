#pragma once

#include <enet.h>

#include <memory>

#include "core/net/net.hpp"

class ENetClient
{
   public:
    ENetClient()                              = default;
    ENetClient(ENetClient &&)                 = default;
    ENetClient(const ENetClient &)            = delete;
    ENetClient &operator=(ENetClient &&)      = default;
    ENetClient &operator=(const ENetClient &) = delete;
    ~ENetClient();

    void Init();
    void Run();

   private:
    enet_host_ptr mClient;
};
