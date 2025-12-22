#pragma once

#include <glm/glm.hpp>

#include "core/serialize.hpp"

struct Projectile {
    float        Damage;
    float        Speed;
    entt::entity Target{entt::null};

    bool Archive(auto& aArchive)
    {
        if (!ArchiveValue(aArchive, Damage, 0.0f, 100.0f)) return false;
        if (!ArchiveValue(aArchive, Speed, 0.0f, 10.0f)) return false;
        return ArchiveEntity(aArchive, Target);
    }
};
