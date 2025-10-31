#pragma once

#include "core/serialize.hpp"

struct Health {
    float Health;

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