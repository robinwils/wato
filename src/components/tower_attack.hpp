#pragma once

#include <entt/entity/entity.hpp>

#include "core/serialize.hpp"

struct TowerAttack {
    float        Range;
    float        FireRate;
    float        TimeSinceLastShot{0.0f};
    entt::entity CurrentTarget{entt::null};

    bool Archive(auto& aArchive)
    {
        if (!ArchiveValue(aArchive, Range, 0.0f, 20.0f)) return false;
        if (!ArchiveValue(aArchive, FireRate, 0.0f, 10.0f)) return false;
        if (!ArchiveValue(aArchive, TimeSinceLastShot, 0.0f, 100.0f)) return false;
        return ArchiveEntity(aArchive, CurrentTarget);
    }
};
