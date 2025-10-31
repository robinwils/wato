#pragma once

#include <doctest.h>

#include <glm/detail/qualifier.hpp>
#include <glm/gtx/string_cast.hpp>

#include "core/sys/log.hpp"

namespace doctest
{
template <glm::length_t L, typename T, glm::qualifier Q>
struct StringMaker<glm::vec<L, T, Q>> {
    static String convert(const glm::vec<L, T, Q>& aValue)
    {
        return String(fmt::format("{}", aValue).c_str());
    }
};
}  // namespace doctest
