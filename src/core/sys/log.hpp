#pragma once

#include <bx/debug.h>
#include <bx/string.h>
#include <fmt/ranges.h>
#include <fmt/std.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <entt/entity/fwd.hpp>
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

template <>
struct fmt::formatter<entt::entity> : fmt::formatter<std::string> {
    auto format(entt::entity const& aObj, format_context& aCtx) const -> decltype(aCtx.out())
    {
        return fmt::format_to(aCtx.out(), "entity #{}", fmt::underlying(aObj));
    }
};

inline void InitLogger(const std::string& aLevel)
{
    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto fileSink    = std::make_shared<spdlog::sinks::basic_file_sink_mt>("wato_logs.txt", true);
    spdlog::level::level_enum level = spdlog::level::from_str(aLevel);

    consoleSink->set_level(level);
    fileSink->set_level(level);
    // FIXME: weird segfault when using %s and %# instead of %@
    // or puting the thread info in separate []
    consoleSink->set_pattern("[%H:%M:%S %z thread %t] [%^%L%$] %v %@");
    fileSink->set_pattern("[%H:%M:%S %z thread %t] [%^%L%$] %v %@");

    std::vector<spdlog::sink_ptr> sinks{consoleSink, fileSink};
    auto logger = std::make_shared<spdlog::logger>("multi_sink", sinks.begin(), sinks.end());
    spdlog::set_default_logger(logger);
    logger->set_level(level);
}

#define WATO_TRACE(...) SPDLOG_TRACE(__VA_ARGS__)
#define WATO_DBG(...)   SPDLOG_DEBUG(__VA_ARGS__)
