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

bgfx::ShaderHandle  loadShader(bx::FileReaderI* _reader, const char* _name);
bgfx::ProgramHandle loadProgram(bx::FileReader* fr, const char* _vsName, const char* _fsName);

struct BlinnPhongMaterial : Material {
    BlinnPhongMaterial(entt::resource<Shader> _shader,
        entt::resource<bgfx::TextureHandle>   _diffuse,
        entt::resource<bgfx::TextureHandle>   _specular)
        : Material(_shader),
          m_diffuseTexture(_diffuse),
          m_specularTexture(_specular),
          m_useDiffuseTexture(true),
          m_useSpecularTexture(true)
    {
    }

    BlinnPhongMaterial(entt::resource<Shader> _shader, glm::vec3 _diffuse, glm::vec3 _specular)
        : Material(_shader),
          m_diffuse(_diffuse),
          m_specular(_specular),
          m_useDiffuseTexture(false),
          m_useSpecularTexture(false)
    {
    }

    void submit() const
    {
        if (m_useDiffuseTexture) {
            bgfx::setTexture(0, shader->uniform("s_diffuseTex"), m_diffuseTexture);
            bgfx::setUniform(shader->uniform("u_diffuse"),
                glm::value_ptr(glm::vec4(m_diffuse, 1.0f)));
        } else {
            bgfx::setUniform(shader->uniform("u_diffuse"),
                glm::value_ptr(glm::vec4(m_diffuse, 0.0f)));
        }

        if (m_useSpecularTexture) {
            bgfx::setTexture(1, shader->uniform("s_specularTex"), m_specularTexture);
            bgfx::setUniform(shader->uniform("u_specular"),
                glm::value_ptr(glm::vec4(m_specular, 1.0f)));
        } else {
            bgfx::setUniform(shader->uniform("u_specular"),
                glm::value_ptr(glm::vec4(m_specular, 0.0f)));
        }
    }

   protected:
    glm::vec3                           m_diffuse;
    bool                                m_useDiffuseTexture;
    entt::resource<bgfx::TextureHandle> m_diffuseTexture;

    glm::vec3                           m_specular;
    bool                                m_useSpecularTexture;
    entt::resource<bgfx::TextureHandle> m_specularTexture;
};
