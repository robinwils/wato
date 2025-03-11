#pragma once

#include <bimg/decode.h>

#include <core/model_loader.hpp>
#include <core/sys.hpp>
#include <entt/core/hashed_string.hpp>
#include <entt/resource/cache.hpp>
#include <renderer/bgfx_utils.hpp>
#include <renderer/material.hpp>
#include <renderer/shader.hpp>

#include "bgfx/bgfx.h"
#include "renderer/primitive.hpp"

using namespace entt::literals;

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

                const bgfx::Memory* mem = bgfx::makeRef(imageContainer->m_data,
                    imageContainer->m_size,
                    imageReleaseCb,
                    imageContainer);
                bx::free(&allocator, data);

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
    }

   private:
    bx::DefaultAllocator allocator;
    bx::FileReader       fr;
};

struct ProgramLoader final {
    using result_type = std::shared_ptr<Shader>;

    template <typename... Args>
    result_type operator()(const char*                           _vsName,
        const char*                                              _fsName,
        std::unordered_map<std::string, bgfx::UniformType::Enum> _uniforms)
    {
        auto handle = loadProgram(&fr, _vsName, _fsName);

        auto uniform_handles = std::unordered_map<std::string, bgfx::UniformHandle>();
        for (auto&& [name, type] : _uniforms) {
            DBG("creating uniform '%s'", name.c_str());
            uniform_handles[name] = bgfx::createUniform(name.c_str(), type);
        }

        return std::make_shared<Shader>(handle, uniform_handles);
    }

   private:
    bx::FileReader fr;
};

using TextureCache = entt::resource_cache<bgfx::TextureHandle, TextureLoader>;
using ProgramCache = entt::resource_cache<Shader, ProgramLoader>;
using ModelCache =
    entt::resource_cache<std::vector<Primitive<PositionNormalUvVertex>*>, ModelLoader>;

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
