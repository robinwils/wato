#pragma once

#include <enet.h>

#include <atomic>
#include <chrono>
#include <optional>

#include "core/net/enet_base.hpp"
#include "core/serialize.hpp"
#include "core/wato.hpp"

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
    void EnqueueSend(NetworkEvent* aPkt) { mQueue.push(aPkt); }
    bool Connect();
    void Disconnect();
    void ForceDisconnect();
    void ConsumeNetworkEvents();

    [[nodiscard]] bool Connected() const noexcept { return mConnected; }

   protected:
    void OnConnect(ENetEvent& aEvent) override;
    void OnReceive(ENetEvent& aEvent) override;
    void OnDisconnect(ENetEvent& aEvent) override;
    void OnDisconnectTimeout(ENetEvent& aEvent) override;
    void OnNone(ENetEvent& aEvent) override;

   private:
    void send(const std::vector<uint8_t> aData);

    ENetPeer*                     mPeer;
    std::atomic_bool              mConnected;
    std::vector<WatoGameInstance> mGameInstances;
};
