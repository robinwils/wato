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

bgfx::ShaderHandle  loadShader(bx::FileReaderI* aReader, const char* aName);
bgfx::ProgramHandle loadProgram(bx::FileReader* aFr, const char* aVsName, const char* aFsName);

class BlinnPhongMaterial : public Material
{
   public:
    BlinnPhongMaterial(
        entt::resource<Shader>              aShader,
        entt::resource<bgfx::TextureHandle> aDiffuse,
        entt::resource<bgfx::TextureHandle> aSpecular,
        bool                                aSkinned = false)
        : Material(aShader),
          mUseDiffuseTexture(true),
          mDiffuseTexture(aDiffuse),
          mUseSpecularTexture(true),
          mSpecularTexture(aSpecular),
          mSkinned(aSkinned)
    {
    }
    virtual ~BlinnPhongMaterial() {}

    BlinnPhongMaterial(entt::resource<Shader> aShader, glm::vec3 aDiffuse, glm::vec3 aSpecular)
        : Material(aShader),
          mDiffuse(aDiffuse),
          mUseDiffuseTexture(false),
          mSpecular(aSpecular),
          mUseSpecularTexture(false)
    {
    }

    void Submit() const
    {
        if (mUseDiffuseTexture) {
            bgfx::setTexture(0, mShader->Uniform("s_diffuseTex"), mDiffuseTexture);
            bgfx::setUniform(
                mShader->Uniform("u_diffuse"),
                glm::value_ptr(glm::vec4(mDiffuse, 1.0f)));
        } else {
            bgfx::setUniform(
                mShader->Uniform("u_diffuse"),
                glm::value_ptr(glm::vec4(mDiffuse, 0.0f)));
        }

        if (mUseSpecularTexture) {
            bgfx::setTexture(1, mShader->Uniform("s_specularTex"), mSpecularTexture);
            bgfx::setUniform(
                mShader->Uniform("u_specular"),
                glm::value_ptr(glm::vec4(mSpecular, 1.0f)));
        } else {
            bgfx::setUniform(
                mShader->Uniform("u_specular"),
                glm::value_ptr(glm::vec4(mSpecular, 0.0f)));
        }
    }

   protected:
    glm::vec3                           mDiffuse;
    bool                                mUseDiffuseTexture;
    entt::resource<bgfx::TextureHandle> mDiffuseTexture;

    glm::vec3                           mSpecular;
    bool                                mUseSpecularTexture;
    entt::resource<bgfx::TextureHandle> mSpecularTexture;

    bool mSkinned;
};
