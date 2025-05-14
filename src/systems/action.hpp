#pragma once

#include <stdexcept>

#include "input/action.hpp"
#include "systems/system.hpp"

class ActionContextHandler
{
   public:
    ActionContextHandler()          = default;
    virtual ~ActionContextHandler() = default;

    virtual void
    operator()(Registry& aRegistry, const MovePayload& aPayload, const float aDeltaTime) = 0;
    virtual void operator()(Registry& aRegistry, const SendCreepPayload& aPayload)       = 0;
    virtual void operator()(Registry& aRegistry, const BuildTowerPayload& aPayload)      = 0;
    virtual void operator()(Registry& aRegistry, const PlacementModePayload& aPayload)   = 0;
};

class DefaultContextHandler : public ActionContextHandler
{
   public:
    DefaultContextHandler()  = default;
    ~DefaultContextHandler() = default;

    void operator()(Registry& aRegistry, const MovePayload& aPayload, const float aDeltaTime)
        override;
    void operator()(Registry& aRegistry, const SendCreepPayload& aPayload) override;
    void operator()(Registry&, const BuildTowerPayload&) override {};
    void operator()(Registry& aRegistry, const PlacementModePayload& aPayload) override;
    void ExitPlacement(Registry& aRegistry);
};

class PlacementModeContextHandler : public DefaultContextHandler
{
   public:
    PlacementModeContextHandler()  = default;
    ~PlacementModeContextHandler() = default;

    void operator()(Registry&, const SendCreepPayload&) override {};
    void operator()(Registry& aRegistry, const BuildTowerPayload& aPayload) override;
    void operator()(Registry& aRegistry, const PlacementModePayload& aPayload) override;
};

class ServerContextHandler : public ActionContextHandler
{
   public:
    ServerContextHandler()  = default;
    ~ServerContextHandler() = default;

    void operator()(Registry&, const SendCreepPayload&) override {};
    void operator()(Registry& aRegistry, const BuildTowerPayload& aPayload) override;
    void operator()(Registry&, const PlacementModePayload&) override {}
    void operator()(Registry&, const MovePayload&, const float) override {}
};

template <typename Derived>
class ActionSystem : public System<Derived>
{
   private:
    void handleAction(Registry& aRegistry, const Action& aAction, const float aDeltaTime);
    void processActions(Registry& aRegistry, ActionTag aFilterTag, const float aDeltaTime);
    void handleContext(
        Registry&             aRegistry,
        const Action&         aAction,
        ActionContextHandler& aCtxHandler,
        const float           aDeltaTime);

    void exitPlacement(Registry& aRegistry);

    // Small function kept inline
    void initializeContextStack(Registry& aRegistry)
    {
        auto& contextStack = aRegistry.ctx().get<ActionContextStack&>();
        if (contextStack.empty()) {
            contextStack.push_front(ActionContext{
                .State    = ActionContext::State::Default,
                .Bindings = ActionBindings::Defaults(),
                .Payload  = NormalPayload{}});
        }
    }

    friend Derived;
};

class RealTimeActionSystem : public ActionSystem<RealTimeActionSystem>
{
   public:
    // Small operator kept inline
    void operator()(Registry& aRegistry, const float aDeltaTime)
    {
        initializeContextStack(aRegistry);
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
        initializeContextStack(aRegistry);
        processActions(aRegistry, ActionTag::FixedTime, aDeltaTime);
    }

    static constexpr const char* StaticName() { return "DeterministicActionSystem"; }
};

class ServerActionSystem : public ActionSystem<ServerActionSystem>
{
   public:
    void operator()(Registry& aRegistry, const float aDeltaTime)
    {
        auto& contextStack = aRegistry.ctx().get<ActionContextStack&>();
        if (contextStack.empty()) {
            contextStack.push_front(ActionContext{
                .State    = ActionContext::State::Server,
                .Bindings = ActionBindings::Defaults(),
                .Payload  = NormalPayload{}});
        }
        processActions(aRegistry, ActionTag::FixedTime, aDeltaTime);
    }

    static constexpr const char* StaticName() { return "ServerActionSystem"; }
};
