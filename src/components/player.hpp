#pragma once

#include <charconv>
#include <cstdint>
#include <entt/entity/fwd.hpp>
#include <expected>
#include <string>

#include "core/types.hpp"

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
