#pragma once

#include <bgfx/embedded_shader.h>
#include <bx/bx.h>
#include <bx/file.h>
#include <essl/fs_blinnphong.sc.bin.h>
#include <essl/fs_grid.sc.bin.h>
#include <essl/fs_simple.sc.bin.h>
#include <essl/vs_blinnphong.sc.bin.h>
#include <essl/vs_blinnphong_skinned.sc.bin.h>
#include <essl/vs_blinnphong_instanced.sc.bin.h>
#include <essl/vs_grid.sc.bin.h>
#include <essl/vs_simple.sc.bin.h>
#include <glsl/fs_blinnphong.sc.bin.h>
#include <glsl/fs_grid.sc.bin.h>
#include <glsl/fs_simple.sc.bin.h>
#include <glsl/vs_blinnphong.sc.bin.h>
#include <glsl/vs_blinnphong_skinned.sc.bin.h>
#include <glsl/vs_blinnphong_instanced.sc.bin.h>
#include <glsl/vs_grid.sc.bin.h>
#include <glsl/vs_simple.sc.bin.h>
#include <spirv/fs_blinnphong.sc.bin.h>
#include <spirv/fs_grid.sc.bin.h>
#include <spirv/fs_simple.sc.bin.h>
#include <spirv/vs_blinnphong.sc.bin.h>
#include <spirv/vs_blinnphong_skinned.sc.bin.h>
#include <spirv/vs_blinnphong_instanced.sc.bin.h>
#include <spirv/vs_grid.sc.bin.h>
#include <spirv/vs_simple.sc.bin.h>

#include <memory>
#include <unordered_map>

#include "renderer/shader.hpp"
#if defined(_WIN32)
#include <dx11/fs_blinnphong.sc.bin.h>
#include <dx11/fs_grid.sc.bin.h>
#include <dx11/fs_simple.sc.bin.h>
#include <dx11/vs_blinnphong.sc.bin.h>
#include <dx11/vs_blinnphong_skinned.sc.bin.h>
#include <dx11/vs_blinnphong_instanced.sc.bin.h>
#include <dx11/vs_grid.sc.bin.h>
#include <dx11/vs_simple.sc.bin.h>
#else
// BX marks linux as a supported platform for Direct3D shaders but shaderc CMake wrapper does not
// generate shader headers for Direct3D on Linux so we need to override the following macro
#undef BGFX_EMBEDDED_SHADER_DXBC
#define BGFX_EMBEDDED_SHADER_DXBC(...)
#endif  //  defined(_WIN32)
#if __APPLE__
#include <metal/fs_blinnphong.sc.bin.h>
#include <metal/fs_grid.sc.bin.h>
#include <metal/fs_simple.sc.bin.h>
#include <metal/vs_blinnphong.sc.bin.h>
#include <metal/vs_blinnphong_skinned.sc.bin.h>
#include <metal/vs_blinnphong_instanced.sc.bin.h>
#include <metal/vs_grid.sc.bin.h>
#include <metal/vs_simple.sc.bin.h>
#endif  // __APPLE__
        //

static const bgfx::EmbeddedShader kEmbeddedShaders[] = {
    BGFX_EMBEDDED_SHADER(vs_simple),
    BGFX_EMBEDDED_SHADER(fs_simple),
    BGFX_EMBEDDED_SHADER(vs_grid),
    BGFX_EMBEDDED_SHADER(fs_grid),
    BGFX_EMBEDDED_SHADER(vs_blinnphong),
    BGFX_EMBEDDED_SHADER(vs_blinnphong_skinned),
    BGFX_EMBEDDED_SHADER(vs_blinnphong_instanced),
    BGFX_EMBEDDED_SHADER(fs_blinnphong),
    BGFX_EMBEDDED_SHADER_END()};

struct ShaderLoader final {
    using uniform_desc_map = std::unordered_map<std::string, UniformDesc>;
    using result_type      = std::shared_ptr<Shader>;

    result_type operator()(
        const char*             aVsName,
        const char*             aFsName   = "",
        const uniform_desc_map& aUniforms = {});
};
