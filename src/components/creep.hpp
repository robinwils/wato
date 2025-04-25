#pragma once

#include <string_view>
enum class CreepType {
    Simple,
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

struct Creep {
    CreepType Type;
};
