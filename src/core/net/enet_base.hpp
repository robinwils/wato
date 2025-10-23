#pragma once

#include <bx/spscqueue.h>

#include <atomic>
#include <entt/signal/dispatcher.hpp>
#include <entt/signal/emitter.hpp>

#include "core/net/net.hpp"
#include "core/queue/channel.hpp"
#include "core/sys/log.hpp"
#include "registry/registry.hpp"

class ENetBase
{
   public:
    using channel_response_t = Channel<NetworkResponse>;
    using channel_request_t  = Channel<NetworkRequest>;

    ENetBase(Logger aLogger) : mRunning(true), mLogger(aLogger) {}
    ENetBase(Logger aLogger, bool aRunning) : mRunning(aRunning), mLogger(aLogger) {}
    ENetBase(ENetBase&&)                 = delete;
    ENetBase(const ENetBase&)            = delete;
    ENetBase& operator=(ENetBase&&)      = delete;
    ENetBase& operator=(const ENetBase&) = delete;
    virtual ~ENetBase();

    virtual void Init();

    // blocking: meant to be called in a dedicated thread
    virtual void Poll();

    [[nodiscard]] bool Running() const noexcept { return mRunning; }

    template <typename Func>
    void ConsumeNetworkResponses(Func&& aHandler)
    {
        mRespChannel.Drain(aHandler);
    }
    template <typename Func>
    void ConsumeNetworkRequests(Func&& aHandler)
    {
        mReqChannel.Drain(aHandler);
    }

    void EnqueueResponse(NetworkResponse* aEvent) { mRespChannel.Send(aEvent); }
    void EnqueueRequest(NetworkRequest* aEvent) { mReqChannel.Send(aEvent); }

   protected:
    bool Send(ENetPeer* aPeer, const std::vector<uint8_t> aData);

    virtual void OnConnect(ENetEvent& aEvent)           = 0;
    virtual void OnReceive(ENetEvent& aEvent)           = 0;
    virtual void OnDisconnect(ENetEvent& aEvent)        = 0;
    virtual void OnDisconnectTimeout(ENetEvent& aEvent) = 0;
    virtual void OnNone(ENetEvent& aEvent)              = 0;

    std::atomic_bool     mRunning;
    enet_host_ptr        mHost;
    bx::DefaultAllocator mAlloc;

    channel_request_t  mReqChannel;
    channel_response_t mRespChannel;

    Logger mLogger;
};
