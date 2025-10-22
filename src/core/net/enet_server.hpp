#pragma once

#include <unordered_map>

#include "core/net/enet_base.hpp"

class ENetServer : public ENetBase
{
    using peer_map = std::unordered_map<PlayerID, ENetPeer*>;

   public:
    ENetServer(const std::string& aSrvAddr) : mServerAddr(aSrvAddr) {}
    ENetServer(ENetServer&&)                 = delete;
    ENetServer(const ENetServer&)            = delete;
    ENetServer& operator=(ENetServer&&)      = delete;
    ENetServer& operator=(const ENetServer&) = delete;
    ~ENetServer()                            = default;

    void Init() override;

    bool Send(PlayerID aID, const std::vector<uint8_t> aData)
    {
        if (!mConnectedPeers.contains(aID)) {
            return false;
        } else {
            return ENetBase::Send(mConnectedPeers[aID], aData);
        }
    }

   protected:
    virtual void OnConnect(ENetEvent& aEvent) override;
    virtual void OnReceive(ENetEvent& aEvent) override;
    virtual void OnDisconnect(ENetEvent& aEvent) override;
    virtual void OnDisconnectTimeout(ENetEvent& aEvent) override;
    virtual void OnNone(ENetEvent& aEvent) override;

   private:
    peer_map    mConnectedPeers;
    std::string mServerAddr;
};
