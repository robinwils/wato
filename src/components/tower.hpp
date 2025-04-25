#pragma once

#include <string_view>
enum class TowerType {
    Arrow,
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

struct Tower {
    TowerType Type;
};
