#include "core/sys.hpp"

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

    DBG("Failed to load %s.", aFilePath);
    return NULL;
}

void* load(bx::FileReaderI* aReader,
    bx::AllocatorI*         aAllocator,
    const char*             aFilePath,
    uint32_t*               aSize)
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
        DBG("Failed to open: %s.", aFilePath);
    }

    if (NULL != aSize) {
        *aSize = 0;
    }

    return NULL;
}
