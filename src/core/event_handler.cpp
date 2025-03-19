#include "core/event_handler.hpp"

#include "components/placement_mode.hpp"
#include "core/sys.hpp"
#include "entt/entity/fwd.hpp"

void EventHandler::onContact(const rp3d::CollisionCallback::CallbackData& callbackData) {}

void EventHandler::onTrigger(const rp3d::OverlapCallback::CallbackData& callbackData)
{
    auto& dispatcher = mRegistry->ctx().get<entt::dispatcher&>();

    for (uint32_t pairIdx = 0; pairIdx < callbackData.getNbOverlappingPairs(); ++pairIdx) {
        const auto& pair    = callbackData.getOverlappingPair(pairIdx);
        const auto* entity1 = static_cast<entt::entity*>(pair.getBody1()->getUserData());
        const auto* entity2 = static_cast<entt::entity*>(pair.getBody2()->getUserData());

        // check if we are triggering the placement mode's ghost tower
        auto placement = mRegistry->view<PlacementMode>();
        bool placementModeTrigger =
            ((entity1 != nullptr) && mRegistry->valid(*entity1) && placement->contains(*entity1))
            || ((entity2 != nullptr) && mRegistry->valid(*entity2)
                && placement->contains(*entity2));

        if (placementModeTrigger) {
            switch (pair.getEventType()) {
                case rp3d::OverlapCallback::OverlapPair::EventType::OverlapStart:
                case rp3d::OverlapCallback::OverlapPair::EventType::OverlapStay:
                    mActionSystem->setCanBuild(false);
                    break;
                case rp3d::OverlapCallback::OverlapPair::EventType::OverlapExit:
                    mActionSystem->setCanBuild(true);
                    break;
            }
        }
    }
}
