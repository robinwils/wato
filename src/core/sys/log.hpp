#pragma once

#include <bx/debug.h>
#include <bx/string.h>
#include <fmt/ranges.h>
#include <fmt/std.h>
#include <spdlog/spdlog.h>

#include <glm/gtx/string_cast.hpp>

template <typename T>
struct fmt::formatter<std::unique_ptr<T>> : fmt::formatter<std::string> {
    auto format(std::unique_ptr<T> const& aObj, format_context& aCtx) const -> decltype(aCtx.out())
    {
        if (aObj) {
            return fmt::format_to(aCtx.out(), "{}", *aObj);
        } else {
            return fmt::format_to(aCtx.out(), "(null unique_ptr)");
        }
    }
};

template <glm::length_t L, typename T, glm::qualifier Q>
struct fmt::formatter<glm::vec<L, T, Q>> : fmt::formatter<std::string> {
    auto format(glm::vec<L, T, Q> const& aObj, format_context& aCtx) const -> decltype(aCtx.out())
    {
        return fmt::format_to(aCtx.out(), "{}", glm::to_string(aObj));
    }
};

template <glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
struct fmt::formatter<glm::mat<C, R, T, Q>> : fmt::formatter<std::string> {
    auto format(glm::mat<C, R, T, Q> const& aObj, format_context& aCtx) const
        -> decltype(aCtx.out())
    {
        return fmt::format_to(aCtx.out(), "{}", glm::to_string(aObj));
    }
};

template <typename T, glm::qualifier Q>
struct fmt::formatter<glm::qua<T, Q>> : fmt::formatter<std::string> {
    auto format(glm::qua<T, Q> const& aObj, format_context& aCtx) const -> decltype(aCtx.out())
    {
        return fmt::format_to(aCtx.out(), "{}", glm::to_string(aObj));
    }
};

#define TRACE(...) SPDLOG_TRACE(__VA_ARGS__)
#define DBG(...)   SPDLOG_DEBUG(__VA_ARGS__)
#define INFO(...)  SPDLOG_INFO(__VA_ARGS__)
#define WARN(...)  SPDLOG_WARN(__VA_ARGS__)
#define ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
