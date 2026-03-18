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

    const CryptoKeys::Public RawPublicKey() const { return mKeys.RawPublicKey(); }

    void SetServerPK(const CryptoKeys::Public& aPubKey) { mServerPK = PublicKey(aPubKey); }

    // Must be called from the network thread
    void ResetSession()
    {
        if (mPeer != nullptr && mPeer->data != nullptr) {
            auto* state = static_cast<PeerState*>(mPeer->data);
            state->SecureSession.Reset();
            state->PeerPK            = mServerPK;
            state->AwaitingHandshake = false;
        }
    }

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
