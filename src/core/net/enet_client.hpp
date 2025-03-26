#pragma once

#include <enet.h>

#include <atomic>
#include <chrono>
#include <optional>

#include "core/net/enet_base.hpp"

using clock_type = std::chrono::steady_clock;

class ENetClient : public ENetBase
{
   public:
    ENetClient() : ENetBase(), mPeer(nullptr), mConnected(false) {}
    ENetClient(ENetClient&&)                 = delete;
    ENetClient(const ENetClient&)            = delete;
    ENetClient& operator=(ENetClient&&)      = delete;
    ENetClient& operator=(const ENetClient&) = delete;
    ~ENetClient()                            = default;

    virtual void Init();
    void         Send();
    bool         Connect();
    void         Disconnect();
    void         ForceDisconnect();

    [[nodiscard]] bool Connected() const noexcept { return mConnected; }

   private:
    void OnConnect(ENetEvent& aEvent);
    void OnReceive(ENetEvent& aEvent, bx::SpScUnboundedQueueT<NetEvent>& aQueue);
    void OnDisconnect(ENetEvent& aEvent);
    void OnDisconnectTimeout(ENetEvent& aEvent);
    void OnNone(ENetEvent& aEvent);

    std::optional<clock_type::time_point> mDiscTimerStart;

    ENetPeer*        mPeer;
    std::atomic_bool mConnected;
};
