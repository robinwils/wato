#include "core/event_handler.hpp"

#include <fmt/base.h>

#include "components/placement_mode.hpp"
#include "core/window.hpp"
#include "entt/entity/fwd.hpp"

void EventHandler::onContact(const rp3d::CollisionCallback::CallbackData& aCallbackData) {}

void EventHandler::onTrigger(const rp3d::OverlapCallback::CallbackData& aCallbackData)
{
    // check if we are triggering the placement mode's ghost tower
    auto placement = mRegistry->view<PlacementMode>();

    for (uint32_t pairIdx = 0; pairIdx < aCallbackData.getNbOverlappingPairs(); ++pairIdx) {
        const auto& pair  = aCallbackData.getOverlappingPair(pairIdx);
        const auto* data1 = static_cast<RigidBodyData*>(pair.getBody1()->getUserData());
        const auto* data2 = static_cast<RigidBodyData*>(pair.getBody2()->getUserData());

        bool entity1IsGhost = (data1 != nullptr) && mRegistry->valid(data1->Entity)
                              && placement->contains(data1->Entity);
        bool entity2IsGhost = (data2 != nullptr) && mRegistry->valid(data2->Entity)
                              && placement->contains(data2->Entity);

        if (data1) {
            fmt::println("entity1 = {:d}", static_cast<uint32_t>(data1->Entity));
            fmt::println("valid entity1 = {}", mRegistry->valid(data1->Entity));
            fmt::println("entity1 placement mode = {}", placement.contains(data1->Entity));
        }
        if (data2) {
            fmt::println("entity2 = {:d}", static_cast<uint32_t>(data2->Entity));
            fmt::println("valid entity2 = {}", mRegistry->valid(data2->Entity));
            fmt::println("entity2 placement mode = {}", placement.contains(data2->Entity));
        }
        if (entity1IsGhost || entity2IsGhost) {
            fmt::println("triggered placement mode");
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
