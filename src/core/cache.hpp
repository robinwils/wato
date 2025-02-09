#pragma once

#include <bimg/decode.h>

#include <core/model_loader.hpp>
#include <core/sys.hpp>
#include <entt/resource/cache.hpp>
#include <renderer/bgfx_utils.hpp>
#include <renderer/material.hpp>

#define TEXTURE_CACHE (ResourceCache::instance().textureCache)
#define PROGRAM_CACHE (ResourceCache::instance().programCache)
#define MODEL_CACHE   (ResourceCache::instance().modelCache)

static void imageReleaseCb(void* _ptr, void* _userData)
{
    BX_UNUSED(_ptr);
    bimg::ImageContainer* imageContainer = (bimg::ImageContainer*)_userData;
    bimg::imageFree(imageContainer);
}

struct TextureLoader final {
    using result_type = std::shared_ptr<bgfx::TextureHandle>;

    template <typename... Args>
    result_type operator()(const char* _name,
        uint64_t                       _flags       = BGFX_TEXTURE_NONE | BGFX_SAMPLER_NONE,
        uint8_t                        _skip        = 0,
        bgfx::TextureInfo*             _info        = NULL,
        bimg::Orientation::Enum*       _orientation = NULL)
    {
        BX_UNUSED(_skip);
        bgfx::TextureHandle handle = BGFX_INVALID_HANDLE;

        uint32_t size;
        void*    data = load(&fr, &allocator, _name, &size);
        if (NULL != data) {
            bimg::ImageContainer* imageContainer = bimg::imageParse(&allocator, data, size);

            if (NULL != imageContainer) {
                if (NULL != _orientation) {
                    *_orientation = imageContainer->m_orientation;
                }

                const bgfx::Memory* mem =
                    bgfx::makeRef(imageContainer->m_data, imageContainer->m_size, imageReleaseCb, imageContainer);
                BX_FREE(&allocator, data);

                if (imageContainer->m_cubeMap) {
                    handle = bgfx::createTextureCube(uint16_t(imageContainer->m_width),
                        1 < imageContainer->m_numMips,
                        imageContainer->m_numLayers,
                        bgfx::TextureFormat::Enum(imageContainer->m_format),
                        _flags,
                        mem);
                } else if (1 < imageContainer->m_depth) {
                    handle = bgfx::createTexture3D(uint16_t(imageContainer->m_width),
                        uint16_t(imageContainer->m_height),
                        uint16_t(imageContainer->m_depth),
                        1 < imageContainer->m_numMips,
                        bgfx::TextureFormat::Enum(imageContainer->m_format),
                        _flags,
                        mem);
                } else if (bgfx::isTextureValid(0,
                               false,
                               imageContainer->m_numLayers,
                               bgfx::TextureFormat::Enum(imageContainer->m_format),
                               _flags)) {
                    handle = bgfx::createTexture2D(uint16_t(imageContainer->m_width),
                        uint16_t(imageContainer->m_height),
                        1 < imageContainer->m_numMips,
                        imageContainer->m_numLayers,
                        bgfx::TextureFormat::Enum(imageContainer->m_format),
                        _flags,
                        mem);
                }

                if (bgfx::isValid(handle)) {
                    DBG("Loaded texture %s", _name);
                    bgfx::setName(handle, _name);
                }

                if (NULL != _info) {
                    bgfx::calcTextureSize(*_info,
                        uint16_t(imageContainer->m_width),
                        uint16_t(imageContainer->m_height),
                        uint16_t(imageContainer->m_depth),
                        imageContainer->m_cubeMap,
                        1 < imageContainer->m_numMips,
                        imageContainer->m_numLayers,
                        bgfx::TextureFormat::Enum(imageContainer->m_format));
                }
            }
        }

        return std::make_shared<bgfx::TextureHandle>(handle);
        // TODO default loader uses std::make_ref as a return type, should it be
        // bgfx::makeRef(loadTexture...) here ? Or smth else
        // return std::make_shared<bgfx::TextureHandle>(loadTexture(_name, _flags, _skip, _info,
        // _orientation));
    }

   private:
    bx::DefaultAllocator allocator;
    bx::FileReader       fr;
};

struct ProgramLoader final {
    using result_type = std::shared_ptr<bgfx::ProgramHandle>;

    template <typename... Args>
    result_type operator()(const char* _vsName, const char* _fsName)
    {
        return std::make_shared<bgfx::ProgramHandle>(loadProgram(&fr, _vsName, _fsName));
    }

   private:
    bx::FileReader fr;
};

using TextureCache = entt::resource_cache<bgfx::TextureHandle, TextureLoader>;
using ProgramCache = entt::resource_cache<bgfx::ProgramHandle, ProgramLoader>;
using ModelCache   = entt::resource_cache<MeshPrimitive, ModelLoader>;

struct ResourceCache {
    static ResourceCache& instance()
    {
        static ResourceCache rc;

        return rc;
    }

    ResourceCache(ResourceCache const&)  = delete;
    void operator=(ResourceCache const&) = delete;

    TextureCache textureCache;
    ProgramCache programCache;
    ModelCache   modelCache;

   private:
    ResourceCache() {};
};
