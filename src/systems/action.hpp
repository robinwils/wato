#pragma once

#include "input/action.hpp"
#include "systems/system.hpp"

class ActionContextHandler
{
   public:
    virtual ~ActionContextHandler() = default;

    virtual void
    operator()(Registry& aRegistry, const MovePayload& aPayload, const float aDeltaTime) = 0;
    virtual void operator()(Registry& aRegistry, SendCreepPayload& aPayload)             = 0;
    virtual void operator()(Registry& aRegistry, BuildTowerPayload& aPayload)            = 0;
    virtual void operator()(Registry& aRegistry, const PlacementModePayload& aPayload)   = 0;
};

class DefaultContextHandler : public ActionContextHandler
{
   public:
    virtual void
    operator()(Registry& aRegistry, const MovePayload& aPayload, const float aDeltaTime) override;
    void operator()(Registry& aRegistry, SendCreepPayload& aPayload) override;
    void operator()(Registry&, BuildTowerPayload&) override {};
    void operator()(Registry& aRegistry, const PlacementModePayload& aPayload) override;
    void ExitPlacement(Registry& aRegistry);
};

class PlacementModeContextHandler : public DefaultContextHandler
{
   public:
    void operator()(Registry&, SendCreepPayload&) override {};
    void operator()(Registry& aRegistry, BuildTowerPayload& aPayload) override;
    void operator()(Registry& aRegistry, const PlacementModePayload& aPayload) override;
};

class ServerContextHandler : public DefaultContextHandler
{
   public:
    void operator()(Registry& aRegistry, SendCreepPayload& aPayload) override;
    void operator()(Registry& aRegistry, BuildTowerPayload& aPayload) override;
    void operator()(Registry&, const PlacementModePayload&) override {}
    void operator()(Registry&, const MovePayload&, const float) override {}
};

class ActionSystem
{
   protected:
    void HandleAction(Registry& aRegistry, Action& aAction, const float aDeltaTime);
    void ProcessActions(Registry& aRegistry, ActionTag aFilterTag, const float aDeltaTime);
    void HandleContext(
        Registry&             aRegistry,
        Action&               aAction,
        ActionContextHandler& aCtxHandler,
        const float           aDeltaTime);

    void ExitPlacement(Registry& aRegistry);
};

/**
 * @brief Real-time action system (frame time)
 *
 * Processes frame-time actions (camera movement, UI interactions).
 * Runs at variable frame rate.
 */
class RealTimeActionSystem : public FrameSystem, public ActionSystem
{
   public:
    using FrameSystem::FrameSystem;

   protected:
    void Execute(Registry& aRegistry, float aDelta) override
    {
        ProcessActions(aRegistry, ActionTag::FrameTime, aDelta);
    }
};

/**
 * @brief Deterministic action system (fixed timestep)
 *
 * Processes fixed-time actions (gameplay inputs, builds, spawns).
 * Runs at deterministic 60 FPS.
 */
class DeterministicActionSystem : public FixedSystem, public ActionSystem
{
   public:
    using FixedSystem::FixedSystem;

   protected:
    void Execute(Registry& aRegistry, [[maybe_unused]] std::uint32_t aTick) override
    {
        constexpr float kTimeStep = 1.0f / 60.0f;
        ProcessActions(aRegistry, ActionTag::FixedTime, kTimeStep);
    }
};

/**
 * @brief Server action system (fixed timestep)
 *
 * Processes server-side actions with authority.
 * Runs at deterministic 60 FPS.
 */
class ServerActionSystem : public FixedSystem, public ActionSystem
{
   public:
    using FixedSystem::FixedSystem;

   protected:
    void Execute(Registry& aRegistry, [[maybe_unused]] std::uint32_t aTick) override
    {
        constexpr float kTimeStep = 1.0f / 60.0f;
        ProcessActions(aRegistry, ActionTag::FixedTime, kTimeStep);
    }
};
