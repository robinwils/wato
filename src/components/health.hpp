#pragma once

#include "core/serialize.hpp"

struct Health {
    float Health;

    bool Archive(auto& aArchive) { return ArchiveValue(aArchive, Health, 0.0f, 100.0f); }
};
