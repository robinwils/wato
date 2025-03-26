#pragma once

#include <bx/spscqueue.h>

#include <atomic>
#include <entt/signal/dispatcher.hpp>
#include <entt/signal/emitter.hpp>

#include "core/net/net.hpp"

class ENetBase
{
   public:
    ENetBase() : mRunning(true), mQueue(&mAlloc) {}
    ENetBase(ENetBase&&)                 = delete;
    ENetBase(const ENetBase&)            = delete;
    ENetBase& operator=(ENetBase&&)      = default;
    ENetBase& operator=(const ENetBase&) = delete;
    virtual ~ENetBase();

    virtual void Init() = 0;
    virtual void Poll(bx::SpScUnboundedQueueT<NetEvent>& aQueue);

    [[nodiscard]] bool Running() const noexcept { return mRunning; }

   protected:
    virtual void OnConnect(ENetEvent& aEvent)                                            = 0;
    virtual void OnReceive(ENetEvent& aEvent, bx::SpScUnboundedQueueT<NetEvent>& aQueue) = 0;
    virtual void OnDisconnect(ENetEvent& aEvent)                                         = 0;
    virtual void OnDisconnectTimeout(ENetEvent& aEvent)                                  = 0;
    virtual void OnNone(ENetEvent& aEvent)                                               = 0;

    std::atomic_bool                  mRunning;
    enet_host_ptr                     mHost;
    bx::DefaultAllocator              mAlloc;
    bx::SpScUnboundedQueueT<NetEvent> mQueue;
};
