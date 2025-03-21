#pragma once

#include <bimg/decode.h>

#include <core/model_loader.hpp>
#include <core/sys.hpp>
#include <entt/core/hashed_string.hpp>
#include <entt/resource/cache.hpp>
#include <memory>
#include <renderer/material.hpp>
#include <renderer/shader.hpp>
#include <string>
#include <unordered_map>
#include <vector>

#include "bgfx/bgfx.h"
#include "bgfx/defines.h"
#include "bimg/bimg.h"
#include "bx/allocator.h"
#include "bx/bx.h"
#include "bx/file.h"
#include "renderer/primitive.hpp"

using namespace entt::literals;

#define WATO_TEXTURE_CACHE (ResourceCache::Instance().TextureCache)
#define WATO_PROGRAM_CACHE (ResourceCache::Instance().ShaderCache)
#define WATO_MODEL_CACHE   (ResourceCache::Instance().ModelCache)

inline static void imageReleaseCb(void* aPtr, void* aUserData)
{
    BX_UNUSED(aPtr);
    auto* imageContainer = (bimg::ImageContainer*)aUserData;
    bimg::imageFree(imageContainer);
}

struct TextureLoader final {
    using result_type = std::shared_ptr<bgfx::TextureHandle>;

    template <typename... Args>
    result_type operator()(const char* aName,
        uint64_t                       aFlags       = BGFX_TEXTURE_NONE | BGFX_SAMPLER_NONE,
        uint8_t                        aSkip        = 0,
        bgfx::TextureInfo*             aInfo        = nullptr,
        bimg::Orientation::Enum*       aOrientation = nullptr)
    {
        BX_UNUSED(aSkip);
        bgfx::TextureHandle handle = BGFX_INVALID_HANDLE;

        uint32_t size = 0;
        void*    data = load(&mfr, &mallocator, aName, &size);
        if (nullptr != data) {
            bimg::ImageContainer* imageContainer = bimg::imageParse(&mallocator, data, size);

            if (nullptr != imageContainer) {
                if (nullptr != aOrientation) {
                    *aOrientation = imageContainer->m_orientation;
                }

                const bgfx::Memory* mem = bgfx::makeRef(imageContainer->m_data,
                    imageContainer->m_size,
                    imageReleaseCb,
                    imageContainer);
                bx::free(&mallocator, data);

                if (imageContainer->m_cubeMap) {
                    handle = bgfx::createTextureCube(uint16_t(imageContainer->m_width),
                        1 < imageContainer->m_numMips,
                        imageContainer->m_numLayers,
                        bgfx::TextureFormat::Enum(imageContainer->m_format),
                        aFlags,
                        mem);
                } else if (1 < imageContainer->m_depth) {
                    handle = bgfx::createTexture3D(uint16_t(imageContainer->m_width),
                        uint16_t(imageContainer->m_height),
                        uint16_t(imageContainer->m_depth),
                        1 < imageContainer->m_numMips,
                        bgfx::TextureFormat::Enum(imageContainer->m_format),
                        aFlags,
                        mem);
                } else if (bgfx::isTextureValid(0,
                               false,
                               imageContainer->m_numLayers,
                               bgfx::TextureFormat::Enum(imageContainer->m_format),
                               aFlags)) {
                    handle = bgfx::createTexture2D(uint16_t(imageContainer->m_width),
                        uint16_t(imageContainer->m_height),
                        1 < imageContainer->m_numMips,
                        imageContainer->m_numLayers,
                        bgfx::TextureFormat::Enum(imageContainer->m_format),
                        aFlags,
                        mem);
                }

                if (bgfx::isValid(handle)) {
                    DBG("Loaded texture %s", aName);
                    bgfx::setName(handle, aName);
                }

                if (nullptr != aInfo) {
                    bgfx::calcTextureSize(*aInfo,
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
    bx::DefaultAllocator mallocator;
    bx::FileReader       mfr;
};

struct ProgramLoader final {
    using result_type = std::shared_ptr<Shader>;

    template <typename... Args>
    result_type operator()(const char*                                  aVsName,
        const char*                                                     aFsName,
        const std::unordered_map<std::string, bgfx::UniformType::Enum>& aUniforms)
    {
        auto handle = loadProgram(&mfr, aVsName, aFsName);

        auto uniformHandles = std::unordered_map<std::string, bgfx::UniformHandle>();
        for (auto&& [name, type] : aUniforms) {
            DBG("creating uniform '%s'", name.c_str());
            uniformHandles[name] = bgfx::createUniform(name.c_str(), type);
        }

        return std::make_shared<Shader>(handle, uniformHandles);
    }

   private:
    bx::FileReader mfr;
};

using TexCache = entt::resource_cache<bgfx::TextureHandle, TextureLoader>;
using ShCache  = entt::resource_cache<Shader, ProgramLoader>;
using MCache   = entt::resource_cache<std::vector<Primitive<PositionNormalUvVertex>*>, ModelLoader>;

struct ResourceCache {
    static ResourceCache& Instance()
    {
        static ResourceCache rc;

        return rc;
    }

    ResourceCache(ResourceCache const&)  = delete;
    void operator=(ResourceCache const&) = delete;

    TexCache TextureCache;
    ShCache  ShaderCache;
    MCache   ModelCache;

   private:
    ResourceCache() {};
};
