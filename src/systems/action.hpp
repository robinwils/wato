#pragma once

#include "input/action.hpp"
#include "systems/system.hpp"

template <typename Derived>
class ActionSystem : public System<Derived>
{
   private:
    void handleAction(Registry& aRegistry, const Action& aAction, const float aDeltaTime);
    void processActions(Registry& aRegistry, ActionTag aFilterTag, const float aDeltaTime);
    void handleDefaultContext(Registry& aRegistry, const Action& aAction, const float aDeltaTime);
    void handlePlacementContext(Registry& aRegistry, const Action& aAction, const float aDeltaTime);
    void handleMovement(Registry& aRegistry, const MovePayload& aPayload, const float aDeltaTime);
    void transitionToPlacement(Registry& aRegistry, const PlacementModePayload& aPayload);
    void exitPlacement(Registry& aRegistry);

    // Small function kept inline
    void initializeContextStack(Registry& aRegistry)
    {
        auto& contextStack = aRegistry.ctx().get<ActionContextStack&>();
        if (contextStack.empty()) {
            contextStack.push_front(ActionContext{.State = ActionContext::State::Default,
                .Bindings                                = ActionBindings::Defaults(),
                .Payload                                 = NormalPayload{}});
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
