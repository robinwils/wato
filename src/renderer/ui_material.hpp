#pragma once
#include <bgfx/bgfx.h>
#include <bx/file.h>
#include <imgui.h>

#include <entt/resource/resource.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "glm/ext/vector_float3.hpp"
#include "renderer/material.hpp"
#include "renderer/shader.hpp"

class UIMaterial : public Material
{
   public:
    UIMaterial(
        entt::resource<Shader>              aShader,
        entt::resource<bgfx::TextureHandle> aTexture)
        : Material(aShader), mTex(aTexture)
    {
    }

    virtual ~UIMaterial() { bgfx::destroy(mTex); }

    void Submit() const override
    {
        BX_ASSERT(bgfx::isValid(mShader->Program()), "grid preview shader handle is invalid");
        BX_ASSERT(
            mTex.handle() != nullptr && bgfx::isValid(mTex),
            "grid preview texture handle is invalid");

        bgfx::setTexture(0, mShader->Uniform("s_gridTex"), mTex);
    }

   protected:
    entt::resource<bgfx::TextureHandle> mTex;
};
