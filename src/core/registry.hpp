#pragma once
#include <entt/entity/registry.hpp>
#include <bimg/decode.h>
#include <bgfx/bgfx.h>


bgfx::TextureHandle loadTexture(const char* _name, uint64_t _flags = BGFX_TEXTURE_NONE | BGFX_SAMPLER_NONE, uint8_t _skip = 0, bgfx::TextureInfo* _info = NULL, bimg::Orientation::Enum* _orientation = NULL);

struct Registry : public entt::basic_registry<entt::entity>
{
	void spawnPlane();
};
