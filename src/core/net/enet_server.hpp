#pragma once

#include "core/net/enet_base.hpp"

class ENetServer : public ENetBase
{
   public:
    ENetServer()                             = default;
    ENetServer(ENetServer&&)                 = default;
    ENetServer(const ENetServer&)            = delete;
    ENetServer& operator=(ENetServer&&)      = default;
    ENetServer& operator=(const ENetServer&) = delete;
    ~ENetServer()                            = default;

    void Init();

   protected:
    virtual void OnConnect(ENetEvent& aEvent);
    virtual void OnReceive(ENetEvent& aEvent, bx::SpScUnboundedQueueT<NetEvent>& aQueue);
    virtual void OnDisconnect(ENetEvent& aEvent);
    virtual void OnDisconnectTimeout(ENetEvent& aEvent);
    virtual void OnNone(ENetEvent& aEvent);
};
