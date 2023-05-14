#pragma once
#include <bgfx/bgfx.h>
#include <bx/file.h>
#include <glm/ext/vector_float4.hpp>
#include <glm/gtc/type_ptr.hpp>

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
		bgfx::setUniform(u_lightDir, glm::value_ptr(glm::vec4(-1.0f, -1.0f, 0.0f, 0.0f)));
		bgfx::setUniform(u_lightCol, glm::value_ptr(glm::vec4(0.5f, 0.5f, 0.5f, 0.0f)));

		bgfx::setTexture(0, s_diffuseTex, diffuseTexture);
	}

	bgfx::UniformHandle u_lightDir;
	bgfx::UniformHandle u_lightCol;
	bgfx::UniformHandle s_diffuseTex;
	bgfx::ProgramHandle program;
	bgfx::TextureHandle diffuseTexture;
	bgfx::TextureHandle specularTexture;
};

