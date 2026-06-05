#pragma once

#include "core/serialize.hpp"

struct Health {
    float    Health;
    PlayerID LastHitBy{};

    bool Archive(auto& aArchive)
    {
        if (!ArchiveValue(aArchive, Health, 0.0f, 100.0f)) return false;
        return ArchivePlayerID(aArchive, LastHitBy);
    }
};
