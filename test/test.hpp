#pragma once

#include <doctest.h>

#include <glm/detail/qualifier.hpp>
#include <glm/gtx/string_cast.hpp>

#include "core/sys/log.hpp"

enum class TestEnum : uint32_t { A = 1, B = 2, C = 3 };

template <>
struct fmt::formatter<TestEnum> : fmt::formatter<std::string> {
    auto format(TestEnum const& aObj, format_context& aCtx) const -> decltype(aCtx.out())
    {
        return fmt::format_to(aCtx.out(), "{}", std::underlying_type_t<TestEnum>(aObj));
    }
};

namespace doctest
{
template <glm::length_t L, typename T, glm::qualifier Q>
struct StringMaker<glm::vec<L, T, Q>> {
    static String convert(const glm::vec<L, T, Q>& aValue)
    {
        return String(fmt::format("{}", aValue).c_str());
    }
};

template <typename T>
struct StringMaker<std::vector<T>> {
    static String convert(const std::vector<T>& aValue)
    {
        return String(fmt::format("{}", aValue).c_str());
    }
};
}  // namespace doctest
