#pragma once

#include "input/action.hpp"
#include "systems/system.hpp"

class ActionContextHandler
{
   public:
    virtual ~ActionContextHandler() = default;

    virtual void
    operator()(Registry& aRegistry, const MovePayload& aPayload, const float aDeltaTime) = 0;
    virtual void operator()(Registry& aRegistry, const SendCreepPayload& aPayload)       = 0;
    virtual void operator()(Registry& aRegistry, BuildTowerPayload& aPayload)            = 0;
    virtual void operator()(Registry& aRegistry, const PlacementModePayload& aPayload)   = 0;
};

class DefaultContextHandler : public ActionContextHandler
{
   public:
    virtual void
    operator()(Registry& aRegistry, const MovePayload& aPayload, const float aDeltaTime) override;
    void operator()(Registry& aRegistry, const SendCreepPayload& aPayload) override;
    void operator()(Registry&, BuildTowerPayload&) override {};
    void operator()(Registry& aRegistry, const PlacementModePayload& aPayload) override;
    void ExitPlacement(Registry& aRegistry);
};

class PlacementModeContextHandler : public DefaultContextHandler
{
   public:
    void operator()(Registry&, const SendCreepPayload&) override {};
    void operator()(Registry& aRegistry, BuildTowerPayload& aPayload) override;
    void operator()(Registry& aRegistry, const PlacementModePayload& aPayload) override;
};

class ServerContextHandler : public DefaultContextHandler
{
   public:
    void operator()(Registry& aRegistry, BuildTowerPayload& aPayload) override;
    void operator()(Registry&, const PlacementModePayload&) override {}
    void operator()(Registry&, const MovePayload&, const float) override {}
};

template <typename Derived>
class ActionSystem : public System<Derived>
{
   private:
    void handleAction(Registry& aRegistry, Action& aAction, const float aDeltaTime);
    void processActions(Registry& aRegistry, ActionTag aFilterTag, const float aDeltaTime);
    void handleContext(
        Registry&             aRegistry,
        Action&               aAction,
        ActionContextHandler& aCtxHandler,
        const float           aDeltaTime);

    void exitPlacement(Registry& aRegistry);

    friend Derived;
};

class RealTimeActionSystem : public ActionSystem<RealTimeActionSystem>
{
   public:
    // Small operator kept inline
    void operator()(Registry& aRegistry, const float aDeltaTime)
    {
        processActions(aRegistry, ActionTag::FrameTime, aDeltaTime);
    }

    static constexpr const char* StaticName() { return "RealTimeActionSystem"; }
};

class DeterministicActionSystem : public ActionSystem<DeterministicActionSystem>
{
   public:
    // Small operator kept inline
    void operator()(Registry& aRegistry, const float aDeltaTime)
    {
        processActions(aRegistry, ActionTag::FixedTime, aDeltaTime);
    }

    static constexpr const char* StaticName() { return "DeterministicActionSystem"; }
};

class ServerActionSystem : public ActionSystem<ServerActionSystem>
{
   public:
    void operator()(Registry& aRegistry, const float aDeltaTime)
    {
        processActions(aRegistry, ActionTag::FixedTime, aDeltaTime);
    }

    static constexpr const char* StaticName() { return "ServerActionSystem"; }
};
