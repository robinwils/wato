#pragma once

#include "reactphysics3d/reactphysics3d.h"
#include "registry/registry.hpp"

struct RigidBodyData {
    RigidBodyData(entt::entity aEntity) : Entity(aEntity) {}
    entt::entity Entity;
};

class EventHandler : public rp3d::EventListener
{
   public:
    EventHandler(Registry* aRegistry) : mRegistry(aRegistry) {}

    /// Called when some trigger events occur
    /**
     * @param callbackData Contains information about all the triggers that are colliding
     */
    void onTrigger(const rp3d::OverlapCallback::CallbackData& /*callbackData*/) override;

   private:
    Registry* mRegistry;
};

class TowerBuildingHandler : public rp3d::EventListener
{
   public:
    /// Called when some contacts occur
    /**
     * @param callbackData Contains information about all the contacts
     */
    void onContact(const rp3d::CollisionCallback::CallbackData& /*callbackData*/) override;

    bool CanBuildTower;
};
