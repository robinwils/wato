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

constexpr int kHexStringBase = 16;

template <typename ID>
    requires(std::is_integral_v<ID> && std::is_unsigned_v<ID>)
inline std::expected<ID, std::errc> IDFromHexString(std::string_view aHexStr)
{
    if (aHexStr.empty()) {
        return std::unexpected(std::errc::invalid_argument);
    }
    ID out{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto [ptr, ec] = std::from_chars(aHexStr.data(), aHexStr.data() + aHexStr.size(), out, kHexStringBase);
    if (ec == std::errc{}) {
        return out;
    }
    return std::unexpected(ec);
}

template <typename ID>
    requires(std::is_integral_v<ID> && std::is_unsigned_v<ID>)
inline std::expected<std::string, std::errc> IDToHexString(ID aID)
{
    static constexpr size_t kSize = 2 * sizeof(ID);

    std::array<char, kSize> buf{};

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto [ptr, ec] = std::to_chars(buf.data(), buf.data() + buf.size(), aID, kHexStringBase);
    if (ec == std::errc{}) {
        return std::string(buf.data(), ptr);
    }
    return std::unexpected(ec);
}

using GameInstanceID = std::uint64_t;
using PlayerID       = uint32_t;

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
