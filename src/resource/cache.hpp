#pragma once

#include <bimg/decode.h>

#include <entt/core/hashed_string.hpp>
#include <entt/resource/cache.hpp>

#include "resource/model_loader.hpp"
#include "resource/shader_loader.hpp"
#include "resource/texture_loader.hpp"

using namespace entt::literals;

using TextureCache = entt::resource_cache<bgfx::TextureHandle, TextureLoader>;
using ShaderCache  = entt::resource_cache<Shader, ShaderLoader>;
using ModelCache   = entt::resource_cache<Model, ModelLoader>;

/**
 * @brief loads a resource in the ressource cache, this forwards to EnTT's resource_cache load
 * function, but also checks if the handle is valid right away. We are using this method instead of
 * the load method directly because it needs to be checked right after load and if not will end in
 * use after free, because the iterator returned by load is invalidated
 *
 * @param aCache cache to load the resource into
 * @param aName string that will be hashed
 * @return entt::resource to the handle
 */
template <typename Cache, typename... Args>
auto LoadResource(Cache& aCache, std::string_view aName, Args&&... aArgs)
{
    auto&& [it, loaded] =
        aCache.load(entt::hashed_string{aName.data()}, std::forward<Args>(aArgs)...);
    auto handle = it->second;
    if (!loaded && !handle) {
        throw std::runtime_error(fmt::format("could not load resource: {}", std::string{aName}));
    }
    return handle;
}
