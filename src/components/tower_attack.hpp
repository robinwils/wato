#pragma once

#include <entt/entity/entity.hpp>
#include <glaze/core/common.hpp>

#include "core/serialize.hpp"

struct TowerAttack {
    float        Range{};
    float        FireRate{};
    float        Damage{};
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

template <>
struct glz::meta<TowerAttack> {
    using T = TowerAttack;

    // NOLINTBEGIN(readability-identifier-naming)
    static constexpr auto value = glz::object(&T::Range, &T::FireRate, &T::Damage);
    // NOLINTEND(readability-identifier-naming)
};
