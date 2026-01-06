#pragma once

#include <reactphysics3d/reactphysics3d.h>

#include <vector>

#include "core/physics/physics.hpp"
#include "core/sys/log.hpp"

struct TriggerEvent {
    rp3d::Collider*                               Collider1;
    rp3d::Collider*                               Collider2;
    rp3d::OverlapCallback::OverlapPair::EventType Event;

    inline std::pair<rp3d::Collider*, rp3d::Collider*> Matches(
        unsigned short aFirstCategory,
        unsigned short aSecondCategory) const
    {
        return MatchColliderPair(Collider1, Collider2, aFirstCategory, aSecondCategory);
    }
};

class PhysicsEventListener : public rp3d::EventListener
{
   public:
    PhysicsEventListener(const Logger& aLogger) : mLogger(aLogger) {}

    void onTrigger(const rp3d::OverlapCallback::CallbackData& aCallbackData) override;

    [[nodiscard]] const std::vector<TriggerEvent>& GetEvents() const { return mEvents; }
    void                                           ClearEvents() { mEvents.clear(); }

   private:
    Logger                    mLogger;
    std::vector<TriggerEvent> mEvents;
};
