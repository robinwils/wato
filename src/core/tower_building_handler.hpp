#pragma once

#include "reactphysics3d/reactphysics3d.h"
#include "registry/registry.hpp"

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
