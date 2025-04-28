#pragma once

struct Health {
    float Health;

    constexpr static auto Serialize(auto& aArchive, const auto& aSelf)
    {
        aArchive.template Write<float>(&aSelf.Health, 1);
    }

    constexpr static auto Deserialize(auto& aArchive, auto& aSelf)
    {
        aArchive.template Read<float>(&aSelf.Health, 1);
    }
};
