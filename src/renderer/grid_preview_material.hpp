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

class GridPreviewMaterial : public Material
{
   public:
    GridPreviewMaterial(
        entt::resource<Shader>              aShader,
        const glm::vec3&                    aInfo,
        entt::resource<bgfx::TextureHandle> aTexture)
        : Material(aShader), mGridInfo(aInfo), mGridTexture(aTexture)
    {
    }

    virtual ~GridPreviewMaterial() {}

    void Submit() const
    {
        bgfx::setTexture(0, mShader->Uniform("s_gridTex"), mGridTexture);
        bgfx::setUniform(mShader->Uniform("u_gridInfo"), glm::value_ptr(mGridInfo));
    }

   protected:
    glm::vec3                           mGridInfo{0.0f};
    entt::resource<bgfx::TextureHandle> mGridTexture;
};
