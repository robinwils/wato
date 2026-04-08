#pragma once

#include <cstdint>
#include <glaze/core/common.hpp>
#include <glaze/core/meta.hpp>
#include <string_view>

enum class CreepType : std::uint8_t {
    Simple,
    Count,
};

[[nodiscard]] constexpr std::string_view CreepTypeToString(CreepType aType)
{
    switch (aType) {
        case CreepType::Simple:
            return "Simple";
        default:
            return "Unknown";
    }
}

template <>
struct glz::meta<CreepType> {
    using enum CreepType;

    // NOLINTBEGIN(readability-identifier-naming)
    static constexpr auto value = glz::enumerate(Simple);
    // NOLINTEND(readability-identifier-naming)
};

struct Creep {
    CreepType Type;
    float     Damage;
};
