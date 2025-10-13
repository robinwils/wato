#pragma once

#include <glm/vec3.hpp>

#include "reactphysics3d/reactphysics3d.h"

class TowerBuildingHandler : public rp3d::OverlapCallback
{
   public:
    TowerBuildingHandler() : CanBuildTower(true) {}

    /// Called when some contacts occur
    /**
     * @param callbackData Contains information about all the contacts
     */
    virtual void onOverlap(CallbackData&) override;

    bool CanBuildTower;

    std::vector<glm::vec3> Contacts;
};
