#include "core/physics/physics_event_listener.hpp"

void PhysicsEventListener::onTrigger(const rp3d::OverlapCallback::CallbackData& aCallbackData)
{
    for (uint32_t i = 0; i < aCallbackData.getNbOverlappingPairs(); ++i) {
        const auto& pair = aCallbackData.getOverlappingPair(i);

        mEvents.push_back(
            TriggerEvent{
                .Collider1 = pair.getCollider1(),
                .Collider2 = pair.getCollider2(),
                .Event     = pair.getEventType(),
            });
    }
}
