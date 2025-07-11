#include "core/event_handler.hpp"

#include <spdlog/spdlog.h>

#include <variant>

#include "components/placement_mode.hpp"
#include "core/physics.hpp"
#include "input/action.hpp"

void EventHandler::onTrigger(const rp3d::OverlapCallback::CallbackData& aCallbackData)
{
    // check if we are triggering the placement mode's ghost tower
    auto placement = mRegistry->view<PlacementMode>();
    bool canBuild  = true;

    for (uint32_t pairIdx = 0; pairIdx < aCallbackData.getNbOverlappingPairs(); ++pairIdx) {
        const auto& pair  = aCallbackData.getOverlappingPair(pairIdx);
        const auto* data1 = static_cast<RigidBodyData*>(pair.getBody1()->getUserData());
        const auto* data2 = static_cast<RigidBodyData*>(pair.getBody2()->getUserData());

        bool entity1IsGhost = (data1 != nullptr) && mRegistry->valid(data1->Entity)
                              && placement->contains(data1->Entity);
        bool entity2IsGhost = (data2 != nullptr) && mRegistry->valid(data2->Entity)
                              && placement->contains(data2->Entity);

        if (entity1IsGhost || entity2IsGhost) {
            switch (pair.getEventType()) {
                case rp3d::OverlapCallback::OverlapPair::EventType::OverlapStart:
                case rp3d::OverlapCallback::OverlapPair::EventType::OverlapStay:
                    canBuild = false;
                    break;
                default:
                    break;
            }
        }
    }
    auto& actionCtx = mRegistry->ctx().get<ActionContextStack&>().front();
    if (auto* payload = std::get_if<PlacementModePayload>(&actionCtx.Payload)) {
        payload->CanBuild = canBuild;
    }
}
