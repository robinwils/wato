#pragma once

#include <glm/vec3.hpp>

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

    std::vector<glm::vec3> Contacts;
};
