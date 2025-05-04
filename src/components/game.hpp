#pragma once

#include "core/types.hpp"

struct GameInstance {
    GameInstanceID GameID;
    float          Accumulator;
    std::uint32_t  Tick;
};
