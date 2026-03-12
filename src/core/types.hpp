#pragma once

#include <SafeInt.hpp>
#include <charconv>
#include <expected>
#include <glaze/core/common.hpp>
#include <glaze/core/meta.hpp>
#include <glaze/core/wrappers.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>
#include <iterator>
#include <span>
#include <string>
#include <vector>

template <>
struct glz::meta<glm::vec3> {
    // NOLINTBEGIN(readability-identifier-naming)
    static constexpr auto value = glz::array(&glm::vec3::x, &glm::vec3::y, &glm::vec3::z);
    // NOLINTEND(readability-identifier-naming)
};

template <>
struct glz::meta<glm::quat> {
    // NOLINTBEGIN(readability-identifier-naming)
    static constexpr auto ReadEuler = [](glm::quat& q, const glm::vec3& degrees) {
        q = glm::quat(glm::radians(degrees));
    };
    static constexpr auto WriteEuler = [](const glm::quat& q) {
        return glm::degrees(glm::eulerAngles(q));
    };
    static constexpr auto value = glz::custom<ReadEuler, WriteEuler>;
    // NOLINTEND(readability-identifier-naming)
};

using GameInstanceID = std::uint64_t;

inline std::expected<GameInstanceID, std::errc> GameIDFromHexString(std::string_view aHexStr)
{
    if (aHexStr.empty()) {
        return std::unexpected(std::errc::invalid_argument);
    }
    GameInstanceID gameID{};
    auto [ptr, ec] = std::from_chars(aHexStr.begin(), aHexStr.end(), gameID, 16);
    if (ec == std::errc{}) {
        return gameID;
    }
    return std::unexpected(ec);
}

inline std::expected<std::string, std::errc> GameIDToHexString(GameInstanceID aID)
{
    static constexpr size_t kSize = sizeof(GameInstanceID);

    std::array<char, kSize> buf{};

    auto [ptr, ec] = std::to_chars(buf.begin(), buf.end(), aID, 2 * kSize);
    if (ec == std::errc{}) {
        return std::string(ptr);
    }
    return std::unexpected(ec);
}

using PlayerID = uint32_t;

inline std::expected<PlayerID, std::errc> PlayerIDFromHexString(std::string_view aHexStr)
{
    if (aHexStr.empty()) {
        return std::unexpected(std::errc::invalid_argument);
    }
    PlayerID id{};
    auto [ptr, ec] = std::from_chars(aHexStr.begin(), aHexStr.end(), id, 16);
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

using byte_view   = std::span<const uint8_t>;
using byte_buffer = std::vector<uint8_t>;
