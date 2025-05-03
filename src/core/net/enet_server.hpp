#pragma once

#include <unordered_map>

#include "core/net/enet_base.hpp"

class ENetServer : public ENetBase
{
    using peer_map = std::unordered_map<PlayerID, ENetPeer*>;

   public:
    ENetServer() : mRespQueue(&mAlloc) {}
    ENetServer(ENetServer&&)                 = delete;
    ENetServer(const ENetServer&)            = delete;
    ENetServer& operator=(ENetServer&&)      = delete;
    ENetServer& operator=(const ENetServer&) = delete;
    ~ENetServer()                            = default;

    void Init() override;
    void EnqueueResponse(NetworkEvent<NetworkResponsePayload>* aPkt) { mRespQueue.push(aPkt); }

   protected:
    virtual void OnConnect(ENetEvent& aEvent) override;
    virtual void OnReceive(ENetEvent& aEvent) override;
    virtual void OnDisconnect(ENetEvent& aEvent) override;
    virtual void OnDisconnectTimeout(ENetEvent& aEvent) override;
    virtual void OnNone(ENetEvent& aEvent) override;

   private:
    peer_map             mConnectedPeers;
    NetworkResponseQueue mRespQueue;
};
