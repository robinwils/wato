#pragma once

#include <charconv>
#include <cstdint>
#include <entt/entity/fwd.hpp>
#include <expected>
#include <string>

using PlayerID = uint32_t;

inline std::expected<PlayerID, std::errc> PlayerIDFromHexString(const std::string& aHexStr)
{
    if (aHexStr.empty()) {
        return std::unexpected(std::errc::invalid_argument);
    }
    PlayerID id{};
    auto [ptr, ec] = std::from_chars(aHexStr.data(), aHexStr.data() + aHexStr.size(), id, 16);
    if (ec == std::errc{}) {
        return id;
    }
    return std::unexpected(ec);
}

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
