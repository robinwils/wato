#pragma once

#include <cstdint>
#include <entt/entity/fwd.hpp>
#include <string>

using PlayerID = uint32_t;

struct Player {
    PlayerID ID;
};

struct Name {
    std::string Username;
};

struct Owner {
    PlayerID ID;
};
