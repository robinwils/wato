#pragma once

#include <cstdint>
#include <entt/entity/fwd.hpp>
#include <string>

using PlayerID = uint32_t;

struct Player {
    PlayerID ID;
};

struct RecordID {
    std::string Value;
};

struct DisplayName {
    std::string Value;
};

struct AccountName {
    std::string Value;
};

struct Email {
    std::string Value;
};

struct Owner {
    PlayerID ID;
};

struct Eliminated {
};
