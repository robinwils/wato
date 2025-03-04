#pragma once

#include <bgfx/bgfx.h>
#include <bx/bx.h>
#include <bx/file.h>

#define DBG_PREFIX "" __FILE__ "(" BX_STRINGIZE(__LINE__) "): WATO "
#define DBG(_format, ...)                                    \
    BX_MACRO_BLOCK_BEGIN                                     \
    bx::debugPrintf(DBG_PREFIX _format "\n", ##__VA_ARGS__); \
    BX_MACRO_BLOCK_END

const bgfx::Memory* loadMem(bx::FileReaderI* _reader, const char* _filePath);
void*               load(bx::FileReaderI* _reader,
                  bx::AllocatorI*         _allocator,
                  const char*             _filePath,
                  uint32_t*               _size = NULL);
