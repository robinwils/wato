#include "core/tower_building_handler.hpp"

#include <spdlog/spdlog.h>

#include "core/physics.hpp"

void TowerBuildingHandler::onContact(const rp3d::CollisionCallback::CallbackData& aCallbackData)
{
    for (uint32_t pairIdx = 0; pairIdx < aCallbackData.getNbContactPairs(); ++pairIdx) {
        rp3d::CollisionCallback::ContactPair pair = aCallbackData.getContactPair(pairIdx);
        if (pair.getCollider1()->getCollisionCategoryBits() == Category::Entities
            || pair.getCollider2()->getCollisionCategoryBits() == Category::Entities) {
            CanBuildTower = true;
            break;
        }
    }
}
