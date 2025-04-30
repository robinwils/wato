#pragma once

#include <cstdint>
#include <entt/entity/fwd.hpp>
#include <string>

using PlayerID = uint32_t;

struct Player {
    PlayerID     ID;
    std::string  Username;
    entt::entity Camera;
};
