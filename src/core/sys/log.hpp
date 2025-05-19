#pragma once

#include <bx/debug.h>
#include <bx/string.h>
#include <spdlog/spdlog.h>

// TODO: real class/lib for logger
#define LOG_PREFIX "" __FILE__ "(" BX_STRINGIZE(__LINE__) "): WATO"

#define TRACE(...) SPDLOG_TRACE(__VA_ARGS__)
#define DBG(...)   SPDLOG_DEBUG(__VA_ARGS__)
#define INFO(...)  SPDLOG_INFO(__VA_ARGS__)
#define WARN(...)  SPDLOG_WARN(__VA_ARGS__)
#define ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
