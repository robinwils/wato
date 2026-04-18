#pragma once

#include <entt/signal/dispatcher.hpp>

#include "core/net/network_events.hpp"
#include "systems/system.hpp"

/**
 * @brief Processes network responses from server (fixed timestep)
 *
 * Handles entity creation, updates, and destruction from RigidBodyUpdateResponse.
 * Handles health synchronization from HealthUpdateResponse.
 * Handles full state sync from SyncPayload.
 *
 * Events are enqueued at frame time via dispatcher.enqueue() and
 * processed here at fixed timestep via dispatcher.update().
 */
class NetworkResponseSystem : public FixedSystem
{
   public:
    using FixedSystem::FixedSystem;

    const char* Name() const override { return "NetworkResponseSystem"; }

   protected:
    void Execute(Registry& aRegistry, std::uint32_t aTick) override;

   private:
    void ensureConnected(entt::dispatcher& aDispatcher);

    // Event handlers (use mRegistry member set during Execute)
    void onRigidBodyUpdate(const RigidBodyUpdateEvent& aEvent);
    void onHealthUpdate(const HealthUpdateEvent& aEvent);
    void onGoldUpdate(const GoldUpdateEvent& aEvent);
    void onSyncPayload(const SyncPayloadEvent& aEvent);

    void createProjectile(
        Registry&                      aRegistry,
        const RigidBodyUpdateResponse& aUpdate,
        const ProjectileInitData&      aInit);
    void createTower(
        Registry&                      aRegistry,
        const RigidBodyUpdateResponse& aUpdate,
        const TowerInitData&           aInit);
    void createCreep(
        Registry&                      aRegistry,
        const RigidBodyUpdateResponse& aUpdate,
        const CreepInitData&           aInit);

    bool mConnected = false;
};
