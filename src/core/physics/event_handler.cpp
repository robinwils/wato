#include "core/physics/event_handler.hpp"

#include <bx/bx.h>
#include <spdlog/spdlog.h>

#include <variant>

#include "components/placement_mode.hpp"
#include "core/physics/physics.hpp"
#include "input/action.hpp"

void EventHandler::onTrigger(const rp3d::OverlapCallback::CallbackData& aCallbackData)
{
    for (uint32_t pairIdx = 0; pairIdx < aCallbackData.getNbOverlappingPairs(); ++pairIdx) {
        const auto& pair = aCallbackData.getOverlappingPair(pairIdx);

        auto* data1 = static_cast<PlacementModeData*>(pair.getBody1()->getUserData());
        auto* data2 = static_cast<PlacementModeData*>(pair.getBody2()->getUserData());

        PlacementModeData* ghostData = nullptr;

        BX_ASSERT(data1 == nullptr || data2 == nullptr, "cannot have 2 ghost towers colliding");

        if (!data1 && !data2) {
            continue;
        }

        ghostData = data1 != nullptr ? data1 : data2;

        if (pair.getEventType() == rp3d::OverlapCallback::OverlapPair::EventType::OverlapStart) {
            ghostData->Overlaps++;
        } else if (
            pair.getEventType() == rp3d::OverlapCallback::OverlapPair::EventType::OverlapExit) {
            ghostData->Overlaps--;
        }
    }
}
