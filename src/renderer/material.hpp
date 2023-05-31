#pragma once
#include <bgfx/bgfx.h>
#include <bx/file.h>
#include <glm/ext/vector_float4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

bgfx::ShaderHandle loadShader(bx::FileReaderI* _reader, const char* _name);
bgfx::ProgramHandle loadProgram(bx::FileReader* fr, const char* _vsName, const char* _fsName);

struct Material
{
	Material(bgfx::ProgramHandle _program = BGFX_INVALID_HANDLE,
		bgfx::TextureHandle _diffuse = BGFX_INVALID_HANDLE,
		bgfx::TextureHandle _specular = BGFX_INVALID_HANDLE)
		: program(_program), diffuseTexture(_diffuse), specularTexture(_specular)
	{
		u_lightDir = bgfx::createUniform("u_lightDir", bgfx::UniformType::Vec4);
		u_lightCol = bgfx::createUniform("u_lightCol", bgfx::UniformType::Vec4);
		s_diffuseTex = bgfx::createUniform("s_diffuseTex", bgfx::UniformType::Sampler);
	}

	void submit() {
		bgfx::setUniform(u_lightDir, glm::value_ptr(glm::vec4(m_lightDir, 0.0f)));
		bgfx::setUniform(u_lightCol, glm::value_ptr(glm::vec4(m_lightCol, 0.0f)));

		bgfx::setTexture(0, s_diffuseTex, diffuseTexture);
	}

	void drawImgui() {
		ImGui::Text("Directional Light settings");
		ImGui::DragFloat3("Light direction", glm::value_ptr(m_lightDir), 0.1f, 5.0f);
		ImGui::DragFloat3("Light color", glm::value_ptr(m_lightCol), 0.10f, 2.0f);
	}

	glm::vec3 m_lightDir = glm::vec3(-1.0f, -1.0f, 0.0f);
	glm::vec3 m_lightCol = glm::vec3(0.5f, 0.5f, 0.5f);
	bgfx::UniformHandle u_lightDir;
	bgfx::UniformHandle u_lightCol;
	bgfx::UniformHandle s_diffuseTex;
	bgfx::ProgramHandle program;
	bgfx::TextureHandle diffuseTexture;
	bgfx::TextureHandle specularTexture;
};

