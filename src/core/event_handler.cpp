#include "core/event_handler.hpp"

#include "components/placement_mode.hpp"
#include "core/window.hpp"
#include "entt/entity/fwd.hpp"

void EventHandler::onContact(const rp3d::CollisionCallback::CallbackData& aCallbackData) {}

void EventHandler::onTrigger(const rp3d::OverlapCallback::CallbackData& aCallbackData)
{
    // check if we are triggering the placement mode's ghost tower
    auto placement = mRegistry->view<PlacementMode>();

    for (uint32_t pairIdx = 0; pairIdx < aCallbackData.getNbOverlappingPairs(); ++pairIdx) {
        const auto& pair    = aCallbackData.getOverlappingPair(pairIdx);
        const auto* entity1 = static_cast<entt::entity*>(pair.getBody1()->getUserData());
        const auto* entity2 = static_cast<entt::entity*>(pair.getBody2()->getUserData());

        bool placementModeTrigger =
            ((entity1 != nullptr) && mRegistry->valid(*entity1) && placement->contains(*entity1))
            || ((entity2 != nullptr) && mRegistry->valid(*entity2)
                && placement->contains(*entity2));

        if (placementModeTrigger) {
            switch (pair.getEventType()) {
                case rp3d::OverlapCallback::OverlapPair::EventType::OverlapStart:
                case rp3d::OverlapCallback::OverlapPair::EventType::OverlapStay:
                    mRegistry->ctx().get<WatoWindow&>().GetInput().Latest().SetCanBuild(false);
                    break;
                case rp3d::OverlapCallback::OverlapPair::EventType::OverlapExit:
                    mRegistry->ctx().get<WatoWindow&>().GetInput().Latest().SetCanBuild(true);
                    break;
            }
        }
    }
}
