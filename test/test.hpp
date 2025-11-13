#pragma once

#include <doctest.h>

#include <glm/detail/qualifier.hpp>
#include <glm/gtx/string_cast.hpp>

#include "core/sys/log.hpp"

#define CHECK_VEC3_APPROX_VALUES(v1, xx, yy, zz)     \
    do {                                             \
        CHECK_EQ(v1.x, doctest::Approx(double(xx))); \
        CHECK_EQ(v1.y, doctest::Approx(double(yy))); \
        CHECK_EQ(v1.z, doctest::Approx(double(zz))); \
    } while (0)

#define CHECK_VEC3_APPROX(v1, v2) CHECK_VEC3_APPROX_EPSILON_VALUES(v1, v2.x, v2.y, v2.z)

#define CHECK_VEC3_APPROX_EPSILON_VALUES(v1, xx, yy, zz, eps)     \
    do {                                                          \
        CHECK_EQ(v1.x, doctest::Approx(double(xx)).epsilon(eps)); \
        CHECK_EQ(v1.y, doctest::Approx(double(yy)).epsilon(eps)); \
        CHECK_EQ(v1.z, doctest::Approx(double(zz)).epsilon(eps)); \
    } while (0)

#define CHECK_VEC3_APPROX_EPSILON(v1, v2, eps) \
    CHECK_VEC3_APPROX_EPSILON_VALUES(v1, v2.x, v2.y, v2.z, eps)

enum class TestEnum : uint32_t { A = 1, B = 2, C = 3, Count };

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
