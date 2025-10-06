#pragma once

#include <enet.h>

#include <atomic>
#include <chrono>
#include <optional>

#include "core/net/enet_base.hpp"
#include "core/serialize.hpp"

class ENetClient : public ENetBase
{
   public:
    ENetClient() : ENetBase(), mConnected(false), mPeer(nullptr) {}
    ENetClient(ENetClient&&)                 = delete;
    ENetClient(const ENetClient&)            = delete;
    ENetClient& operator=(ENetClient&&)      = delete;
    ENetClient& operator=(const ENetClient&) = delete;
    ~ENetClient()                            = default;

    void Init() override;
    void EnqueueSend(NetworkEvent<NetworkRequestPayload>* aPkt) { mQueue.push(aPkt); }
    bool Connect();
    void Disconnect();
    void ForceDisconnect();
    void ConsumeNetworkRequests();

    [[nodiscard]] bool Connected() const noexcept { return mConnected; }

   protected:
    void OnConnect(ENetEvent& aEvent) override;
    /**
     * @brief Raw server response
     *
     * @param aEvent enet structure encapsulating the packet received
     */
    void OnReceive(ENetEvent& aEvent) override;
    void OnDisconnect(ENetEvent& aEvent) override;
    void OnDisconnectTimeout(ENetEvent& aEvent) override;
    void OnNone(ENetEvent& aEvent) override;

    std::atomic_bool mConnected;

   private:
    void send(const std::vector<uint8_t> aData);

    ENetPeer* mPeer;
};
