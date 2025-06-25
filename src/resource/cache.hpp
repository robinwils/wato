#pragma once

#include <bimg/decode.h>

#include <entt/core/hashed_string.hpp>
#include <entt/resource/cache.hpp>

#include "resource/model_loader.hpp"
#include "resource/shader_loader.hpp"
#include "resource/texture_loader.hpp"

using namespace entt::literals;

#define WATO_TEXTURE_CACHE (ResourceCache::Instance().TextureCache)
#define WATO_PROGRAM_CACHE (ResourceCache::Instance().ShaderCache)
#define WATO_MODEL_CACHE   (ResourceCache::Instance().ModelCache)

using texture_cache = entt::resource_cache<bgfx::TextureHandle, TextureLoader>;
using shader_cache  = entt::resource_cache<Shader, ShaderLoader>;
using model_cache   = entt::resource_cache<Model, ModelLoader>;

struct ResourceCache {
    static ResourceCache& Instance()
    {
        static ResourceCache rc;

        return rc;
    }

    ResourceCache(ResourceCache const&)  = delete;
    void operator=(ResourceCache const&) = delete;

    texture_cache TextureCache;
    shader_cache  ShaderCache;
    model_cache   ModelCache;

   private:
    ResourceCache() {};
};
