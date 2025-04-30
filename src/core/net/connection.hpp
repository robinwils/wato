#pragma once

#include <enet.h>

class NetworkConnection
{
   public:
    NetworkConnection()                                    = default;
    NetworkConnection(const NetworkConnection&)            = default;
    NetworkConnection(NetworkConnection&&)                 = default;
    NetworkConnection& operator=(const NetworkConnection&) = default;
    NetworkConnection& operator=(NetworkConnection&&)      = default;
    ~NetworkConnection()                                   = default;

   private:
    ENetPeer* mPeer;
};
