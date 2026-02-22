#pragma once

#include <SafeInt.hpp>
#include <charconv>
#include <expected>
#include <string>

using GameInstanceID = std::uint64_t;

inline std::expected<GameInstanceID, std::errc> GameIDFromHexString(const std::string& aHexStr)
{
    if (aHexStr.empty()) {
        return std::unexpected(std::errc::invalid_argument);
    }
    GameInstanceID gameID{};
    auto [ptr, ec] = std::from_chars(aHexStr.data(), aHexStr.data() + aHexStr.size(), gameID, 16);
    if (ec == std::errc{}) {
        return gameID;
    }
    return std::unexpected(ec);
}

inline std::expected<std::string, std::errc> GameIDToHexString(GameInstanceID aID)
{
    char buf[8]{};

    auto [ptr, ec] = std::to_chars(buf, buf + 8, aID, 16);
    if (ec == std::errc{}) {
        return std::string(ptr);
    }
    return std::unexpected(ec);
}

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

template <class... Ts>
struct VariantVisitor : Ts... {
    using Ts::operator()...;
};

using SafeI32   = SafeInt<int32_t>;
using SafeU16   = SafeInt<uint16_t>;
using SafeU32   = SafeInt<uint32_t>;
using SafeU64   = SafeInt<uint64_t>;
using SafeSizeT = SafeInt<size_t>;

template <typename T>
struct std::hash<SafeInt<T>> {
    std::size_t operator()(const SafeInt<T>& aS) const
    {
        return std::hash<T>()(static_cast<const T>(aS));
    }
};
