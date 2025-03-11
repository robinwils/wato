#pragma once
#include <bgfx/bgfx.h>
#include <bx/file.h>
#include <imgui.h>

#include <entt/resource/resource.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "core/sys.hpp"
#include "glm/ext/vector_float3.hpp"
#include "renderer/shader.hpp"

bgfx::ShaderHandle  loadShader(bx::FileReaderI* _reader, const char* _name);
bgfx::ProgramHandle loadProgram(bx::FileReader* fr, const char* _vsName, const char* _fsName);

struct Material {
    Material(entt::resource<Shader> _shader) : shader(_shader) {}

    virtual void submit() const {}

    entt::resource<Shader> shader;

   protected:
};
