#pragma once

#include <cstdint>

namespace wato {

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

}  // namespace wato
