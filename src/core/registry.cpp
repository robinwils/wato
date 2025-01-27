#include "registry.hpp"
#include <glm/ext/vector_float3.hpp>
#include <components/position.hpp>
#include <components/rotation.hpp>
#include <components/scale.hpp>
#include <components/scene_object.hpp>
#include <renderer/plane.hpp>
#include <core/sys.hpp>
#include <core/cache.hpp>
#include <entt/core/hashed_string.hpp>
#include <components/direction.hpp>
#include <components/color.hpp>

using namespace entt::literals;

bgfx::TextureHandle loadTexture(const char* _filePath, uint64_t _flags, uint8_t _skip, bgfx::TextureInfo* _info, bimg::Orientation::Enum* _orientation)
{
	BX_UNUSED(_skip);
	bgfx::TextureHandle handle = BGFX_INVALID_HANDLE;

	uint32_t size;
	void* data = load(&BxSingleton::getInstance().reader, &BxSingleton::getInstance().allocator, _filePath, &size);
	if (NULL != data)
	{
		bimg::ImageContainer* imageContainer = bimg::imageParse(&BxSingleton::getInstance().allocator, data, size);

		if (NULL != imageContainer)
		{
			if (NULL != _orientation)
			{
				*_orientation = imageContainer->m_orientation;
			}

			const bgfx::Memory* mem = bgfx::makeRef(
				imageContainer->m_data
				, imageContainer->m_size
				, imageReleaseCb
				, imageContainer
			);
			BX_FREE(&BxSingleton::getInstance().allocator, data);
			

			if (imageContainer->m_cubeMap)
			{
				handle = bgfx::createTextureCube(
					uint16_t(imageContainer->m_width)
					, 1 < imageContainer->m_numMips
					, imageContainer->m_numLayers
					, bgfx::TextureFormat::Enum(imageContainer->m_format)
					, _flags
					, mem
				);
			}
			else if (1 < imageContainer->m_depth)
			{
				handle = bgfx::createTexture3D(
					uint16_t(imageContainer->m_width)
					, uint16_t(imageContainer->m_height)
					, uint16_t(imageContainer->m_depth)
					, 1 < imageContainer->m_numMips
					, bgfx::TextureFormat::Enum(imageContainer->m_format)
					, _flags
					, mem
				);
			}
			else if (bgfx::isTextureValid(0, false, imageContainer->m_numLayers, bgfx::TextureFormat::Enum(imageContainer->m_format), _flags))
			{
				handle = bgfx::createTexture2D(
					uint16_t(imageContainer->m_width)
					, uint16_t(imageContainer->m_height)
					, 1 < imageContainer->m_numMips
					, imageContainer->m_numLayers
					, bgfx::TextureFormat::Enum(imageContainer->m_format)
					, _flags
					, mem
				);
			}

			if (bgfx::isValid(handle))
			{
				bgfx::setName(handle, _filePath);
			}

			if (NULL != _info)
			{
				bgfx::calcTextureSize(
					*_info
					, uint16_t(imageContainer->m_width)
					, uint16_t(imageContainer->m_height)
					, uint16_t(imageContainer->m_depth)
					, imageContainer->m_cubeMap
					, 1 < imageContainer->m_numMips
					, imageContainer->m_numLayers
					, bgfx::TextureFormat::Enum(imageContainer->m_format)
				);
			}
		}
	}

	return handle;
}

void Registry::spawnPlane()
{
	//bgfx::ProgramHandle program = loadProgram(&BxFactory::getInstance().reader, "vs_cubes", "fs_cubes");
	auto [bp, pLoaded] = PROGRAM_CACHE.load("blinnphong"_hs, "vs_blinnphong", "fs_blinnphong");

	// TODO: verbose mode using second element of pair
	auto [ loadedDiffuseTexture, dLoaded ] = TEXTURE_CACHE.load("grass/diffuse"_hs, "assets/textures/TreeTop_COLOR.png");
	auto [ loadedSpecularTexture, sLoaded ] = TEXTURE_CACHE.load("grass/specular"_hs, "assets/textures/TreeTop_SPEC.png");

	// FIXME: Cannot use the return of the cache's load function, it crashes, the operator[] does almost the same thing as load, invetigate
	auto diffuse = TEXTURE_CACHE["grass/diffuse"_hs];
	auto specular = TEXTURE_CACHE["grass/specular"_hs];

	// TODO: check validity
	// auto dv = bgfx::isValid(diffuse);
	// auto sv = bgfx::isValid(specular);

	auto program = PROGRAM_CACHE["blinnphong"_hs];

	auto plane = create();
	emplace<Position>(plane, glm::vec3(0.0f));
	emplace<Rotation>(plane, glm::vec3(0.0f));
	emplace<Scale>(plane, glm::vec3(1.0f));
	emplace<SceneObject>(plane, new PlanePrimitive(), Material(program, diffuse, specular));
}

void Registry::spawnMap(uint32_t _w, uint32_t _h)
{
	auto [bp, pLoaded] = PROGRAM_CACHE.load("blinnphong"_hs, "vs_blinnphong", "fs_blinnphong");
	auto[diff, diffLoaded] = TEXTURE_CACHE.load("grass/diffuse"_hs, "assets/textures/TreeTop_COLOR.png");
	auto [sp, sLoaded] = TEXTURE_CACHE.load("grass/specular"_hs, "assets/textures/TreeTop_SPEC.png");

	auto program = PROGRAM_CACHE["blinnphong"_hs];
	auto diffuse = TEXTURE_CACHE["grass/diffuse"_hs];
	auto specular = TEXTURE_CACHE["grass/specular"_hs];


	Material m(program, diffuse, specular);

	for (uint32_t i = 0; i < _w; ++i) {
		for (uint32_t j = 0; j < _h; ++j) {
			auto tile = create();
			emplace<Position>(tile, glm::vec3(i, 0, j));
			emplace<Rotation>(tile, glm::vec3(0, 0, 0));
			emplace<Scale>(tile, glm::vec3(1.0f));
			emplace<SceneObject>(tile, new PlanePrimitive(), m);
		}
	}
}

void Registry::spawnLight()
{
	auto light = create();
	emplace<Direction>(light, glm::vec3(-1.0f, -1.0f, 0.0f));
	emplace<Color>(light, glm::vec3(0.5f));
}
