#include "core/sys/mem.hpp"

#include "core/sys/log.hpp"

const bgfx::Memory* loadMem(bx::FileReaderI* aReader, const char* aFilePath)
{
    if (bx::open(aReader, aFilePath)) {
        uint32_t            size = (uint32_t)bx::getSize(aReader);
        const bgfx::Memory* mem  = bgfx::alloc(size + 1);
        bx::read(aReader, mem->data, size, bx::ErrorAssert{});
        bx::close(aReader);
        mem->data[mem->size - 1] = '\0';
        return mem;
    }

    WATO_DBG("Failed to load {}.", aFilePath);
    return NULL;
}

void* load(
    bx::FileReaderI* aReader,
    bx::AllocatorI*  aAllocator,
    const char*      aFilePath,
    uint32_t*        aSize)
{
    if (bx::open(aReader, aFilePath)) {
        uint32_t size = (uint32_t)bx::getSize(aReader);
        void*    data = bx::alloc(aAllocator, size);
        bx::read(aReader, data, size, bx::ErrorAssert{});
        bx::close(aReader);
        if (NULL != aSize) {
            *aSize = size;
        }
        return data;
    } else {
        WATO_DBG("Failed to open: {}.", aFilePath);
    }

    if (NULL != aSize) {
        *aSize = 0;
    }

    return NULL;
}
