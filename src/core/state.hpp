#pragma once

#include "core/serialize.hpp"
#include "input/action.hpp"

struct GameState {
    uint32_t             Tick{0};
    ActionsType          Actions{};
    std::vector<uint8_t> Snapshot{};

    constexpr static auto Serialize(auto& aArchive, const auto& aSelf)
    {
        ActionsType::size_type nActions = aSelf.Actions.size();

        aArchive.template Write<uint32_t>(&aSelf.Tick, 1);
        aArchive.template Write<ActionsType::size_type>(&nActions, 1);
        for (const Action& action : aSelf.Actions) {
            Action::Serialize(aArchive, action);
        }
        ::Serialize(aArchive, aSelf.Snapshot);
    }

    constexpr static auto Deserialize(auto& aArchive, auto& aSelf)
    {
        ActionsType::size_type nActions = 0;
        aArchive.template Read<uint32_t>(&aSelf.Tick, 1);
        aArchive.template Read<ActionsType::size_type>(&nActions, 1);
        for (ActionsType::size_type idx = 0; idx < nActions; idx++) {
            Action action;
            if (!Action::Deserialize(aArchive, action)) {
                throw std::runtime_error("cannot deserialize action");
            }
            aSelf.Actions.push_back(action);
        }
        ::Deserialize(aArchive, aSelf.Snapshot);
        return true;
    }
};

inline bool operator==(const GameState& aLHS, const GameState& aRHS)
{
    return aLHS.Tick == aRHS.Tick && aLHS.Actions == aRHS.Actions && aLHS.Snapshot == aRHS.Snapshot;
}

using GameStateBuffer = RingBuffer<GameState, 128>;
