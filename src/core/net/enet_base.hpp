#pragma once

#include <bx/spscqueue.h>

#include <atomic>
#include <entt/signal/dispatcher.hpp>
#include <entt/signal/emitter.hpp>

#include "core/net/net.hpp"
#include "registry/registry.hpp"

using NetworkRequestQueue  = bx::SpScUnboundedQueueT<NetworkEvent<NetworkRequestPayload>>;
using NetworkResponseQueue = bx::SpScUnboundedQueueT<NetworkEvent<NetworkResponsePayload>>;

class ENetBase
{
   public:
    ENetBase() : mRunning(true), mQueue(&mAlloc) {}
    ENetBase(ENetBase&&)                 = delete;
    ENetBase(const ENetBase&)            = delete;
    ENetBase& operator=(ENetBase&&)      = delete;
    ENetBase& operator=(const ENetBase&) = delete;
    virtual ~ENetBase();

    virtual void Init();

    // blocking: meant to be called in a dedicated thread
    virtual void Poll();

    [[nodiscard]] bool                 Running() const noexcept { return mRunning; }
    [[nodiscard]] NetworkRequestQueue& Queue() noexcept { return mQueue; }

   protected:
    virtual void OnConnect(ENetEvent& aEvent)           = 0;
    virtual void OnReceive(ENetEvent& aEvent)           = 0;
    virtual void OnDisconnect(ENetEvent& aEvent)        = 0;
    virtual void OnDisconnectTimeout(ENetEvent& aEvent) = 0;
    virtual void OnNone(ENetEvent& aEvent)              = 0;

    std::atomic_bool     mRunning;
    enet_host_ptr        mHost;
    bx::DefaultAllocator mAlloc;
    NetworkRequestQueue  mQueue;
};
