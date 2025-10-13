#include "core/tower_building_handler.hpp"

#include <spdlog/spdlog.h>

#include "core/physics/physics.hpp"

void TowerBuildingHandler::onOverlap(CallbackData& aCallbackData)
{
    spdlog::trace("checking contacts");
    for (uint32_t pairIdx = 0; pairIdx < aCallbackData.getNbOverlappingPairs(); ++pairIdx) {
        OverlapPair pair = aCallbackData.getOverlappingPair(pairIdx);

        unsigned short c1Category = pair.getCollider1()->getCollisionCategoryBits();
        unsigned short c2Category = pair.getCollider2()->getCollisionCategoryBits();

        spdlog::debug("got overlap pair {}: 0x{:x}-0x{:x}", pairIdx, c1Category, c2Category);
        if (c1Category == Category::Entities || c2Category == Category::Entities) {
            CanBuildTower = false;
        }
    }
}
