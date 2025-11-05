#pragma once

#include "core/serialize.hpp"

struct Health {
    float Health;

    bool Archive(auto& aArchive) { return ArchiveValue(aArchive, Health, 0.0f, 100.0f); }

    constexpr static auto Serialize(auto& aArchive, const auto& aSelf)
    {
        ::Serialize(aArchive, aSelf.Health);
    }

    constexpr static auto Deserialize(auto& aArchive, auto& aSelf)
    {
        ::Deserialize(aArchive, aSelf.Health);
        return true;
    }
};
