#pragma once

#include <enet.h>

#include <atomic>

#include "core/net/enet_base.hpp"
#include "core/serialize.hpp"

class ENetClient : public ENetBase
{
   public:
    ENetClient(Logger& aLogger) : ENetBase(aLogger), mConnected(false), mPeer(nullptr) {}
    ENetClient(ENetClient&&)                 = delete;
    ENetClient(const ENetClient&)            = delete;
    ENetClient& operator=(ENetClient&&)      = delete;
    ENetClient& operator=(const ENetClient&) = delete;
    ~ENetClient()                            = default;

    void Init() override;
    bool Connect();
    void Disconnect();
    void ForceDisconnect();

    void Send(std::span<const uint8_t> aData);

    [[nodiscard]] bool Connected() const noexcept { return mConnected; }

    const CryptoKeys::Public PublicKey() const { return mKeys.PublicKey(); }

    void SetServerPK(const CryptoKeys::Public& aPubKey) { mServerPK = ::PublicKey(aPubKey); }

   protected:
    void OnConnect(ENetEvent& aEvent) override;
    /**
     * @brief Raw server response
     *
     * @param aEvent enet structure encapsulating the packet received
     */
    void OnReceive(ENetEvent& aEvent, byte_view aData) override;
    void OnDisconnect(ENetEvent& aEvent) override;
    void OnDisconnectTimeout(ENetEvent& aEvent) override;
    void OnNone(ENetEvent& aEvent) override;

    std::atomic_bool mConnected;

   private:
    ENetPeer* mPeer;

    ::PublicKey mServerPK{};
};
