#include "core/sys.hpp"

const bgfx::Memory* loadMem(bx::FileReaderI* _reader, const char* _filePath)
{
    if (bx::open(_reader, _filePath)) {
        uint32_t            size = (uint32_t)bx::getSize(_reader);
        const bgfx::Memory* mem  = bgfx::alloc(size + 1);
        bx::read(_reader, mem->data, size, bx::ErrorAssert{});
        bx::close(_reader);
        mem->data[mem->size - 1] = '\0';
        return mem;
    }

    DBG("Failed to load %s.", _filePath);
    return NULL;
}

void* load(bx::FileReaderI* _reader,
    bx::AllocatorI*         _allocator,
    const char*             _filePath,
    uint32_t*               _size)
{
    if (bx::open(_reader, _filePath)) {
        uint32_t size = (uint32_t)bx::getSize(_reader);
        void*    data = BX_ALLOC(_allocator, size);
        bx::read(_reader, data, size, bx::ErrorAssert{});
        bx::close(_reader);
        if (NULL != _size) {
            *_size = size;
        }
        return data;
    } else {
        DBG("Failed to open: %s.", _filePath);
    }

    if (NULL != _size) {
        *_size = 0;
    }

    return NULL;
}
