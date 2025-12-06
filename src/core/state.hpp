#pragma once

#include "core/serialize.hpp"
#include "input/action.hpp"

struct GameState {
    uint32_t              Tick{0};
    ActionsType           Actions{};
    std::vector<uint32_t> Snapshot{};

    bool Archive(auto& aArchive)
    {
        if (!ArchiveValue(aArchive, Tick, 0u, 30000000u)) return false;
        if (!ArchiveVector(aArchive, Actions, 32u)) return false;
        if (!ArchiveVector(
                aArchive,
                Snapshot,
                0u,
                std::numeric_limits<uint32_t>::max(),
                (1u << 14)))
            return false;
        return true;
    }
};

inline bool operator==(const GameState& aLHS, const GameState& aRHS)
{
    return aLHS.Tick == aRHS.Tick && aLHS.Actions == aRHS.Actions && aLHS.Snapshot == aRHS.Snapshot;
}

using GameStateBuffer = RingBuffer<GameState, 128>;
