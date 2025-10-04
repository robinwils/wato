#include "shader_loader.hpp"

#include <bgfx/bgfx.h>

#include "core/sys/log.hpp"
#include "core/sys/mem.hpp"

ShaderLoader::result_type ShaderLoader::operator()(
    const char*             aVsName,
    const char*             aFsName,
    const uniform_desc_map& aUniforms)
{
    auto uniformHandles = Shader::uniform_map{};
    for (auto&& [name, desc] : aUniforms) {
        DBG("creating uniform '{}'", name);
        uniformHandles[name] = bgfx::createUniform(name.c_str(), desc.Type, desc.Number);
    }

    bx::FileReader           fr;
    bgfx::RendererType::Enum type = bgfx::getRendererType();

    bgfx::ShaderHandle vsh = bgfx::createEmbeddedShader(kEmbeddedShaders, type, aVsName);

    if (!bgfx::isValid(vsh)) {
        throw std::runtime_error(fmt::format("shader {} is invalid", aVsName));
    }

    bgfx::ShaderHandle fsh = BGFX_INVALID_HANDLE;
    if (aFsName != nullptr) {
        fsh = bgfx::createEmbeddedShader(kEmbeddedShaders, type, aFsName);
        if (!bgfx::isValid(fsh)) {
            throw std::runtime_error(fmt::format("shader {} is invalid", aFsName));
        }
    }

    return std::make_shared<Shader>(bgfx::createProgram(vsh, fsh, true), std::move(uniformHandles));
}
