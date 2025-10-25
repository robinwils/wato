#pragma once

#include <reactphysics3d/reactphysics3d.h>

#include <glm/vec3.hpp>

#include "core/sys/log.hpp"

class TowerBuildingHandler : public rp3d::OverlapCallback
{
   public:
    TowerBuildingHandler(const Logger& aLogger) : CanBuildTower(true), mLogger(aLogger) {}

    /// Called when some contacts occur
    /**
     * @param callbackData Contains information about all the contacts
     */
    virtual void onOverlap(CallbackData&) override;

    bool CanBuildTower;

    std::vector<glm::vec3> Contacts;

   private:
    Logger mLogger;
};
