#pragma once

// TODO: this should not be here, but in renderer
#include <bgfx/bgfx.h>
#include <bx/file.h>

const bgfx::Memory* loadMem(bx::FileReaderI* aReader, const char* aFilePath);
void*               load(bx::FileReaderI* aReader,
                  bx::AllocatorI*         aAllocator,
                  const char*             aFilePath,
                  uint32_t*               aSize = NULL);
