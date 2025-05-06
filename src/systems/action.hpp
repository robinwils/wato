#pragma once

#include <stdexcept>

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
    void handleSendCreep(Registry& aRegistry, const SendCreepPayload& aPayload);
    void handleBuildTower(Registry& aRegistry, const BuildTowerPayload& aPayload);
    void transitionToPlacement(Registry& aRegistry, const PlacementModePayload& aPayload);
    void exitPlacement(Registry& aRegistry);

    // Small function kept inline
    void initializeContextStack(Registry& aRegistry)
    {
        if (!aRegistry.ctx().contains<ActionContextStack>()) {
            throw std::runtime_error("no action context stack");
        }

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
    // Small operator kept inline
    void operator()(Registry& aRegistry, const float aDeltaTime)
    {
        auto&         abuf          = aRegistry.ctx().get<ActionBuffer&>();
        PlayerActions latestActions = abuf.Latest();

        for (const Action& action : latestActions.Actions) {
            // fmt::println("{}", action.String());
            if (action.Tag != ActionTag::FixedTime) {
                continue;
            }
            std::visit(
                VariantVisitor{
                    [&](const PlacementModePayload&) {},
                    [&](const MovePayload&) {},
                    [&](const BuildTowerPayload& aPayload) {
                        handleBuildTower(aRegistry, aPayload);
                    },
                    [&](const SendCreepPayload& aPayload) { handleSendCreep(aRegistry, aPayload); },

                },
                action.Payload);
        }
    }

    static constexpr const char* StaticName() { return "ServerActionSystem"; }
};
