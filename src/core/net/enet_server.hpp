#pragma once

#include "core/net/enet_base.hpp"

class ENetServer : public ENetBase
{
   public:
    ENetServer()                             = default;
    ENetServer(ENetServer&&)                 = delete;
    ENetServer(const ENetServer&)            = delete;
    ENetServer& operator=(ENetServer&&)      = delete;
    ENetServer& operator=(const ENetServer&) = delete;
    ~ENetServer()                            = default;

    void Init() override;
    void ConsumeEvents(Registry* aRegistry) override;

   protected:
    virtual void OnConnect(ENetEvent& aEvent) override;
    virtual void OnReceive(ENetEvent& aEvent) override;
    virtual void OnDisconnect(ENetEvent& aEvent) override;
    virtual void OnDisconnectTimeout(ENetEvent& aEvent) override;
    virtual void OnNone(ENetEvent& aEvent) override;
};
