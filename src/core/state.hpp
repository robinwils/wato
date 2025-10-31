#pragma once

#include "core/serialize.hpp"
#include "input/action.hpp"

struct GameState {
    uint32_t             Tick{0};
    ActionsType          Actions{};
    std::vector<uint8_t> Snapshot{};

    constexpr static auto Serialize(auto& aArchive, const auto& aSelf)
    {
        ::Serialize(aArchive, aSelf.Tick);
        ::Serialize(aArchive, aSelf.Actions);
        ::Serialize(aArchive, aSelf.Snapshot);
    }

    constexpr static auto Deserialize(auto& aArchive, auto& aSelf)
    {
        ::Deserialize(aArchive, aSelf.Tick);
        ::Deserialize(aArchive, aSelf.Actions);
        ::Deserialize(aArchive, aSelf.Snapshot);
        return true;
    }
};

inline bool operator==(const GameState& aLHS, const GameState& aRHS)
{
    return aLHS.Tick == aRHS.Tick && aLHS.Actions == aRHS.Actions && aLHS.Snapshot == aRHS.Snapshot;
}

using GameStateBuffer = RingBuffer<GameState, 128>;