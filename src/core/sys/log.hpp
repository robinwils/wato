#pragma once

#include <bx/debug.h>
#include <bx/string.h>
#include <spdlog/fmt/ranges.h>
#include <spdlog/spdlog.h>

#include <optional>

#include "config.h"

template <typename T>
struct fmt::formatter<std::optional<T>> : fmt::formatter<std::string> {
    auto format(std::optional<T> aObj, format_context& aCtx) const -> decltype(aCtx.out())
    {
        if (aObj.has_value()) {
            return fmt::format_to(aCtx.out(), "optional with value {}", *aObj);
        } else {
            return fmt::format_to(aCtx.out(), "(null optional)");
        }
    }
};

// TODO: real class/lib for logger
#define LOG_PREFIX "" __FILE__ "(" BX_STRINGIZE(__LINE__) "): WATO"

#define TRACE(...) SPDLOG_TRACE(__VA_ARGS__)
#define DBG(...)   SPDLOG_DEBUG(__VA_ARGS__)
#define INFO(...)  SPDLOG_INFO(__VA_ARGS__)
#define WARN(...)  SPDLOG_WARN(__VA_ARGS__)
#define ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
