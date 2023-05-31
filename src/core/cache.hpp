#pragma once
#include <entt/resource/cache.hpp>
#include <renderer/bgfx_utils.hpp>

struct TextureLoader final {
    using result_type = std::shared_ptr<bgfx::TextureHandle>;

    template<typename... Args>
    result_type operator()(const char* _name, uint64_t _flags = BGFX_TEXTURE_NONE | BGFX_SAMPLER_NONE, uint8_t _skip = 0, bgfx::TextureInfo* _info = NULL, bimg::Orientation::Enum* _orientation = NULL) {
        // TODO default loader uses std::make_ref as a return type, should it be
        // bgfx::makeRef(loadTexture...) here ? Or smth else
        return std::make_shared<bgfx::TextureHandle>(loadTexture(_name, _flags, _skip, _info, _orientation));
    }
};

struct my_resource { const int value; };

struct my_loader final {
    using result_type = std::shared_ptr<my_resource>;

    result_type operator()(int value) const {
        // ...
        return std::make_shared<my_resource>(value);
    }
};
using my_cache = entt::resource_cache<my_resource, my_loader>;


using TextureCache = entt::resource_cache<bgfx::TextureHandle, TextureLoader>;

struct ResourceCache
{
    static ResourceCache& instance() {
        static ResourceCache rc;

        return rc;
    }

    ResourceCache(ResourceCache const&) = delete;
    void operator=(ResourceCache const&) = delete;

    TextureCache textureCache;
private:
    ResourceCache() {};
};