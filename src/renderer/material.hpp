#pragma once
#include <bgfx/bgfx.h>
#include <bx/file.h>
#include <imgui.h>

#include <entt/resource/resource.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "renderer/shader.hpp"

class Material
{
   public:
    Material(entt::resource<Shader> aShader) : mShader(aShader) {}
    virtual ~Material() {}

    virtual void Submit() const {}

    bgfx::ProgramHandle Program() const
    {
        BX_ASSERT(
            mShader.handle() != nullptr && bgfx::isValid(mShader->Program()),
            "invalid shader handler");
        return mShader->Program();
    }

   protected:
    entt::resource<Shader> mShader;
};
