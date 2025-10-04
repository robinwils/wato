#pragma once

#include <SafeInt.hpp>

using GameInstanceID = std::uint64_t;

template <class... Ts>
struct VariantVisitor : Ts... {
    using Ts::operator()...;
};

using SafeU16   = SafeInt<uint16_t>;
using SafeU32   = SafeInt<uint32_t>;
using SafeSizeT = SafeInt<size_t>;

template <typename T>
struct std::hash<SafeInt<T> > {
    std::size_t operator()(const SafeInt<T>& aS) const
    {
        return std::hash<T>()(static_cast<const T>(aS));
    }
};
