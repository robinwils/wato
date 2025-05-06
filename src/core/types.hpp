#pragma once

#include <cstdint>

using GameInstanceID = std::uint64_t;

template <class... Ts>
struct VariantVisitor : Ts... {
    using Ts::operator()...;
};
