#pragma once

#include "core/event/event.hpp"

struct CreepSpawnEvent : public Event<CreepSpawnEvent> {
    CreepSpawnEvent()                                   = default;
    CreepSpawnEvent(const CreepSpawnEvent &)            = default;
    CreepSpawnEvent(CreepSpawnEvent &&)                 = default;
    CreepSpawnEvent &operator=(const CreepSpawnEvent &) = default;
    CreepSpawnEvent &operator=(CreepSpawnEvent &&)      = default;
};
