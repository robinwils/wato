#pragma once

#include "input/action.hpp"
#include "systems/system.hpp"

/**
 * @brief Real-time action system (frame time)
 *
 * Processes frame-time actions (camera movement, placement mode toggle).
 * Reads from FrameActionBuffer, clears after processing.
 */
class RealTimeActionSystem : public FrameSystem
{
   public:
    using FrameSystem::FrameSystem;

    const char* Name() const override { return "RealTimeActionSystem"; }

   protected:
    void Execute(Registry& aRegistry, float aDelta) override;

   private:
    void moveCamera(Registry& aRegistry, const MovePayload& aPayload, float aDeltaTime);
};

/**
 * @brief Deterministic action system (fixed timestep)
 *
 * Processes fixed-time actions (gameplay inputs, builds, spawns).
 * Runs at deterministic 60 FPS.
 */
class DeterministicActionSystem : public FixedSystem
{
   public:
    using FixedSystem::FixedSystem;

    const char* Name() const override { return "DeterministicActionSystem"; }

   protected:
    void Execute(Registry& aRegistry, std::uint32_t aTick) override;

   private:
    bool clientValidateTower(Registry& aRegistry, const BuildTowerPayload& aPayload);
};

/**
 * @brief Server action system (fixed timestep)
 *
 * Processes server-side actions with authority.
 * Runs at deterministic 60 FPS.
 */
class ServerActionSystem : public FixedSystem
{
   public:
    using FixedSystem::FixedSystem;

    const char* Name() const override { return "ServerActionSystem"; }

   protected:
    void Execute(Registry& aRegistry, std::uint32_t aTick) override;

   private:
    void serverBuildTower(Registry& aRegistry, BuildTowerPayload& aPayload, PlayerID aPlayerID);
    void serverSendCreep(Registry& aRegistry, SendCreepPayload& aPayload, PlayerID aPlayerID);
};
