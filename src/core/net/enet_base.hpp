#pragma once

#include <bx/spscqueue.h>

#include <entt/signal/dispatcher.hpp>
#include <entt/signal/emitter.hpp>

#include "core/net/net.hpp"

class ENetBase
{
   public:
    ENetBase()                           = default;
    ENetBase(ENetBase&&)                 = default;
    ENetBase(const ENetBase&)            = delete;
    ENetBase& operator=(ENetBase&&)      = default;
    ENetBase& operator=(const ENetBase&) = delete;
    virtual ~ENetBase();

    virtual void Init() = 0;
    virtual void Poll(bx::SpScUnboundedQueueT<NetEvent>& aQueue);

   protected:
    virtual void OnConnect(ENetEvent& aEvent)                                            = 0;
    virtual void OnReceive(ENetEvent& aEvent, bx::SpScUnboundedQueueT<NetEvent>& aQueue) = 0;
    virtual void OnDisconnect(ENetEvent& aEvent)                                         = 0;
    virtual void OnDisconnectTimeout(ENetEvent& aEvent)                                  = 0;
    virtual void OnNone(ENetEvent& aEvent)                                               = 0;

    enet_host_ptr mHost;
};
