#pragma once

#include <cstdint>
#include <entt/entt.hpp>

#include "components/rigid_body.hpp"
#include "core/sys/log.hpp"

// used in callback, lifetime not handled by EnTT
struct PlacementModeData {
    int      Overlaps = 0;
    uint64_t LastTick = 0;
};

struct PlacementMode {
    PlacementModeData* Data;

    static void on_destroy(entt::registry& aRegistry, const entt::entity aEntity)
    {
        spdlog::trace("destroying placement mode component");
        auto& placementMode = aRegistry.get<PlacementMode>(aEntity);
        if (placementMode.Data) {
            delete placementMode.Data;
            placementMode.Data                            = nullptr;
            aRegistry.get<RigidBody>(aEntity).Params.Data = nullptr;
        }
    }
};
