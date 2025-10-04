#pragma once

#include <bgfx/bgfx.h>
#include <bimg/decode.h>
#include <bx/allocator.h>
#include <bx/bx.h>
#include <bx/file.h>

#include <cstdint>
#include <memory>

struct TextureLoader final {
    using result_type = std::shared_ptr<bgfx::TextureHandle>;

    result_type operator()(
        const char*              aName,
        uint64_t                 aFlags       = BGFX_TEXTURE_NONE | BGFX_SAMPLER_NONE,
        uint8_t                  aSkip        = 0,
        bgfx::TextureInfo*       aInfo        = nullptr,
        bimg::Orientation::Enum* aOrientation = nullptr);

    result_type operator()(
        uint16_t                  aWidth,
        uint16_t                  aHeight,
        bool                      aHasMips,
        uint16_t                  aNumLayers,
        bgfx::TextureFormat::Enum aFormat,
        uint64_t                  aFlags = BGFX_TEXTURE_NONE | BGFX_SAMPLER_NONE,
        const void*               aData  = nullptr);
};
