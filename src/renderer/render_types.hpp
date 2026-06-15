#pragma once

#include <bgfx/bgfx.h>

#include <cstdint>

namespace wato
{

using ViewId = uint16_t;

enum class UniformType : uint8_t {
    Sampler,
    Vec4,
    Mat4,
};

enum class TextureFormat : uint8_t {
    R8,
    BGRA8,
};

static constexpr ViewId kRenderPass  = 0;
static constexpr ViewId kPickingPass = 1;

}  // namespace wato
