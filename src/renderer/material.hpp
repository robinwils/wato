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

bgfx::ShaderHandle  loadShader(bx::FileReaderI* aReader, const char* aName);
bgfx::ProgramHandle loadProgram(bx::FileReader* aFr, const char* aVsName, const char* aFsName);

class Material
{
   public:
    Material(entt::resource<Shader> aShader) : mShader(aShader) {}
    virtual ~Material() {}

    virtual void Submit() const {}

    bgfx::ProgramHandle Program() const { return mShader->Program(); }

   protected:
    entt::resource<Shader> mShader;
};
