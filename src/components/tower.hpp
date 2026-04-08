#pragma once

#include <glaze/core/common.hpp>
#include <glaze/core/meta.hpp>
#include <string_view>

enum class TowerType : std::uint8_t {
    Arrow,
    Count,
};

[[nodiscard]] constexpr std::string_view TowerTypeToString(TowerType aType)
{
    switch (aType) {
        case TowerType::Arrow:
            return "Arrow";
        default:
            return "Unknown";
    }
}

template <>
struct glz::meta<TowerType> {
    using enum TowerType;

    // NOLINTBEGIN(readability-identifier-naming)
    static constexpr auto value = glz::enumerate(Arrow);
    // NOLINTEND(readability-identifier-naming)
};

struct Tower {
    TowerType Type;
};
