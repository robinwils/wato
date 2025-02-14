#pragma once
#include <bgfx/bgfx.h>
#include <bx/file.h>
#include <imgui.h>

#include <entt/resource/resource.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "core/sys.hpp"
#include "glm/ext/vector_float3.hpp"

bgfx::ShaderHandle  loadShader(bx::FileReaderI* _reader, const char* _name);
bgfx::ProgramHandle loadProgram(bx::FileReader* fr, const char* _vsName, const char* _fsName);

struct Material {
    Material(entt::resource<bgfx::ProgramHandle> _program,
        entt::resource<bgfx::TextureHandle>      _diffuse,
        entt::resource<bgfx::TextureHandle>      _specular)
        : program(_program),
          m_diffuseTexture(_diffuse),
          m_specularTexture(_specular),
          m_useDiffuseTexture(true),
          m_useSpecularTexture(true)
    {
        init_uniforms();
    }

    Material(entt::resource<bgfx::ProgramHandle> _program, glm::vec3 _diffuse, glm::vec3 _specular)
        : program(_program),
          m_diffuse(_diffuse),
          m_specular(_specular),
          m_useDiffuseTexture(false),
          m_useSpecularTexture(false)
    {
        init_uniforms();
    }

    void submit() const
    {
        bgfx::setUniform(u_lightDir, glm::value_ptr(glm::vec4(m_lightDir, 0.0f)));
        bgfx::setUniform(u_lightCol, glm::value_ptr(glm::vec4(m_lightCol, 0.0f)));

        if (m_useDiffuseTexture) {
            bgfx::setTexture(0, s_diffuseTex, m_diffuseTexture);
            bgfx::setUniform(u_diffuse, glm::value_ptr(glm::vec4(m_diffuse, 1.0f)));
        } else {
            bgfx::setUniform(u_diffuse, glm::value_ptr(glm::vec4(m_diffuse, 0.0f)));
        }

        if (m_useSpecularTexture) {
            bgfx::setTexture(1, s_specularTex, m_specularTexture);
            bgfx::setUniform(u_specular, glm::value_ptr(glm::vec4(m_specular, 1.0f)));
        } else {
            bgfx::setUniform(u_specular, glm::value_ptr(glm::vec4(m_specular, 0.0f)));
        }
    }

    void drawImgui()
    {
        ImGui::Text("Directional Light settings");
        ImGui::DragFloat3("Light direction", glm::value_ptr(m_lightDir), 0.1f, 5.0f);
        ImGui::DragFloat3("Light color", glm::value_ptr(m_lightCol), 0.10f, 2.0f);
    }

    entt::resource<bgfx::ProgramHandle> program;

   protected:
    void init_uniforms()
    {
        s_diffuseTex  = bgfx::createUniform("s_diffuseTex", bgfx::UniformType::Sampler);
        s_specularTex = bgfx::createUniform("s_specularTex", bgfx::UniformType::Sampler);

        // w is a bool indicating to use texture or not
        u_diffuse  = bgfx::createUniform("u_diffuse", bgfx::UniformType::Vec4);
        u_specular = bgfx::createUniform("u_specular", bgfx::UniformType::Vec4);

        u_lightDir = bgfx::createUniform("u_lightDir", bgfx::UniformType::Vec4);
        u_lightCol = bgfx::createUniform("u_lightCol", bgfx::UniformType::Vec4);
    }

    glm::vec3 m_lightDir = glm::vec3(-1.0f, -1.0f, 0.0f);
    glm::vec3 m_lightCol = glm::vec3(0.5f, 0.5f, 0.5f);

    bgfx::UniformHandle u_lightDir;
    bgfx::UniformHandle u_lightCol;

    glm::vec3                           m_diffuse;
    bool                                m_useDiffuseTexture;
    entt::resource<bgfx::TextureHandle> m_diffuseTexture;
    bgfx::UniformHandle                 u_diffuse;
    bgfx::UniformHandle                 s_diffuseTex;

    glm::vec3                           m_specular;
    bool                                m_useSpecularTexture;
    entt::resource<bgfx::TextureHandle> m_specularTexture;
    bgfx::UniformHandle                 u_specular;
    bgfx::UniformHandle                 s_specularTex;
};
