#pragma once

#include <charconv>
#include <cstdint>
#include <entt/entity/fwd.hpp>
#include <expected>
#include <string>

#include "core/types.hpp"

struct Player {
    PlayerID ID;
    uint8_t  Slot;  // 0 based index
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
    uint8_t  Slot;
};

struct Target {
    PlayerID ID;
    uint8_t  Slot;
};

struct Eliminated {
};
