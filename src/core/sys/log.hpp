#pragma once

#include <bx/debug.h>
#include <bx/string.h>

// TODO: real class/lib for logger
#define LOG_PREFIX "" __FILE__ "(" BX_STRINGIZE(__LINE__) "): WATO"

#define DBG_PREFIX LOG_PREFIX "[Debug]: "
#define DBG(_format, ...)                                    \
    BX_MACRO_BLOCK_BEGIN                                     \
    bx::debugPrintf(DBG_PREFIX _format "\n", ##__VA_ARGS__); \
    BX_MACRO_BLOCK_END

#define INFO_PREFIX LOG_PREFIX "[Info]: "
#define INFO(_format, ...)                               \
    BX_MACRO_BLOCK_BEGIN                                 \
    bx::printf(INFO_PREFIX _format "\n", ##__VA_ARGS__); \
    BX_MACRO_BLOCK_END

#define ERROR_PREFIX LOG_PREFIX "[Error]: "
#define ERROR(_format, ...)                               \
    BX_MACRO_BLOCK_BEGIN                                  \
    bx::printf(ERROR_PREFIX _format "\n", ##__VA_ARGS__); \
    BX_MACRO_BLOCK_END
