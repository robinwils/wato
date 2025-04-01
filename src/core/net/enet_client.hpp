#pragma once

#include <enet.h>

#include <atomic>
#include <chrono>
#include <optional>

#include "core/net/enet_base.hpp"

class ENetClient : public ENetBase
{
   public:
    ENetClient() : ENetBase(), mPeer(nullptr), mConnected(false) {}
    ENetClient(ENetClient&&)                 = delete;
    ENetClient(const ENetClient&)            = delete;
    ENetClient& operator=(ENetClient&&)      = delete;
    ENetClient& operator=(const ENetClient&) = delete;
    ~ENetClient()                            = default;

    void Init() override;
    void EnqueueSend(NetEvent* aEvent) { mQueue.push(aEvent); }
    bool Connect();
    void Disconnect();
    void ForceDisconnect();
    void ConsumeEvents(Registry* aRegistry) override;

    [[nodiscard]] bool Connected() const noexcept { return mConnected; }

   protected:
    void OnConnect(ENetEvent& aEvent) override;
    void OnReceive(ENetEvent& aEvent) override;
    void OnDisconnect(ENetEvent& aEvent) override;
    void OnDisconnectTimeout(ENetEvent& aEvent) override;
    void OnNone(ENetEvent& aEvent) override;

   private:
    using clock_type = std::chrono::steady_clock;

    void send(std::string& aEvStr);

    std::optional<clock_type::time_point> mDiscTimerStart;

    ENetPeer*        mPeer;
    std::atomic_bool mConnected;
};
