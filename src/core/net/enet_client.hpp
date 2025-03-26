#pragma once

#include <enet.h>

#include "core/net/enet_base.hpp"

class ENetClient : public ENetBase
{
   public:
    ENetClient() : ENetBase(), mPeer(nullptr) {}
    ENetClient(ENetClient&&)                 = default;
    ENetClient(const ENetClient&)            = delete;
    ENetClient& operator=(ENetClient&&)      = default;
    ENetClient& operator=(const ENetClient&) = delete;
    ~ENetClient()                            = default;

    virtual void Init();
    void         Send();
    bool         Connect();
    void         Disconnect();

   private:
    void OnConnect(ENetEvent& aEvent);
    void OnReceive(ENetEvent& aEvent, bx::SpScUnboundedQueueT<NetEvent>& aQueue);
    void OnDisconnect(ENetEvent& aEvent);
    void OnDisconnectTimeout(ENetEvent& aEvent);
    void OnNone(ENetEvent& aEvent);

    ENetPeer* mPeer;
};
