#include "core/physics/event_handler.hpp"

#include <bx/bx.h>
#include <spdlog/spdlog.h>

void EventHandler::onTrigger(const rp3d::OverlapCallback::CallbackData& aCallbackData)
{
    for (uint32_t pairIdx = 0; pairIdx < aCallbackData.getNbOverlappingPairs(); ++pairIdx) {
        const auto& pair = aCallbackData.getOverlappingPair(pairIdx);

        auto* data1 = pair.getBody1()->getUserData();
        auto* data2 = pair.getBody2()->getUserData();
    }
}
