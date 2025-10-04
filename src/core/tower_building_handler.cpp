#include "core/tower_building_handler.hpp"

#include <spdlog/spdlog.h>

#include "core/physics.hpp"

void TowerBuildingHandler::onContact(const rp3d::CollisionCallback::CallbackData& aCallbackData)
{
    for (uint32_t pairIdx = 0; pairIdx < aCallbackData.getNbContactPairs(); ++pairIdx) {
        rp3d::CollisionCallback::ContactPair pair = aCallbackData.getContactPair(pairIdx);

        unsigned short c1Category = pair.getCollider1()->getCollisionCategoryBits();
        unsigned short c2Category = pair.getCollider2()->getCollisionCategoryBits();

        spdlog::debug(
            "got contact pair {} with {} contact points: 0x{:x}-0x{:x}",
            pairIdx,
            pair.getNbContactPoints(),
            c1Category,
            c2Category);
        if (c1Category == Category::Entities || c2Category == Category::Entities) {
            CanBuildTower = true;
        }
        if (c1Category == Category::Terrain || c2Category == Category::Terrain) {
            for (uint32_t ptIdx = 0; ptIdx < pair.getNbContactPoints(); ++ptIdx) {
                rp3d::CollisionCallback::ContactPoint point = pair.getContactPoint(ptIdx);
                rp3d::Vector3 worldPoint = pair.getCollider1()->getLocalToWorldTransform()
                                           * point.getLocalPointOnCollider1();
                spdlog::debug("  got contact point {}: {}", ptIdx, worldPoint);
                Contacts.emplace_back(worldPoint.x, worldPoint.y, worldPoint.z);
            }
        }
    }
}
