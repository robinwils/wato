#pragma once

#include <unordered_map>

#include "core/net/enet_base.hpp"
#include "core/net/net.hpp"
#include "core/sys/log.hpp"

class PocketBaseClient;

struct AuthResult {
    ENetPeer*   Peer;
    PlayerID    ID;
    std::string AccountName;
};

class ENetServer : public ENetBase
{
    using peer_map = std::unordered_map<PlayerID, ENetPeer*>;

   public:
    ENetServer(const std::string& aSrvAddr, Logger aLogger, PocketBaseClient& aPBClient)
        : ENetBase(aLogger, false), mServerAddr(aSrvAddr), mPBClient(aPBClient)
    {
    }
    ENetServer(ENetServer&&)                 = delete;
    ENetServer(const ENetServer&)            = delete;
    ENetServer& operator=(ENetServer&&)      = delete;
    ENetServer& operator=(const ENetServer&) = delete;
    ~ENetServer()                            = default;

    void Init() override;
    void ProcessAuthResults();

    void BroadcastResponse(
        const std::span<const PlayerID> aPlayers,
        PacketType                      aType,
        uint32_t                        aTick,
        const NetworkResponsePayload&   aPayload)
    {
        for (const PlayerID& id : aPlayers) {
            mRespChannel.Send(new NetworkResponse{
                .Type     = aType,
                .PlayerID = id,
                .Tick     = aTick,
                .Payload  = aPayload,
            });
        }
    }
    bool Send(PlayerID aID, const std::span<const uint8_t> aData)
    {
        if (!mConnectedPeers.contains(aID)) {
            return false;
        } else {
            return ENetBase::Send(mConnectedPeers[aID], aData);
        }
    }

    const std::string& GetAccountName(PlayerID aID) const
    {
        static const std::string empty;

        auto it = mAccountNames.find(aID);
        return it != mAccountNames.end() ? it->second : empty;
    }

    const std::pair<std::string, int> IPAndPort() const
    {
        std::pair<std::string, int> res{mServerAddr, 0};

        const size_t pos = mServerAddr.rfind(":");

        if (pos != std::string::npos) {
            res.first  = mServerAddr.substr(0, pos);
            res.second = std::stoi(mServerAddr.substr(pos + 1));
        }

        if (res.first == "any") res.first = "0.0.0.0";
        return res;
    }

   protected:
    virtual void OnConnect(ENetEvent& aEvent) override;
    virtual void OnReceive(ENetEvent& aEvent) override;
    virtual void OnDisconnect(ENetEvent& aEvent) override;
    virtual void OnDisconnectTimeout(ENetEvent& aEvent) override;
    virtual void OnNone(ENetEvent& aEvent) override;

   private:
    // R/W on the separate network thread, careful
    peer_map            mConnectedPeers;
    std::string         mServerAddr;
    Channel<AuthResult> mAuthResultChan;
    PocketBaseClient&   mPBClient;

    std::unordered_map<PlayerID, std::string> mAccountNames;
};
