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
	bgfx::ProgramHandle program = loadProgram(&BxSingleton::getInstance().reader, "vs_blinnphong", "fs_blinnphong");

	// TODO: verbose mode using second element of pair
	TextureCache textureCache;


	auto loadedDiffuseTexture = textureCache.load("grass/diffuse"_hs, "assets/textures/TreeTop_COLOR.png");
	auto loadedSpecularTexture = textureCache.load("grass/specular"_hs, "assets/textures/TreeTop_SPEC.png");
	entt::resource<bgfx::TextureHandle> diffuse = loadedDiffuseTexture.first->second;
	entt::resource<bgfx::TextureHandle> specular = loadedSpecularTexture.first->second;

	//bgfx::TextureHandle diffuse = loadTexture("assets/textures/TreeTop_COLOR.png");
	//bgfx::TextureHandle specular = loadTexture("assets/textures/TreeTop_SPEC.png");

	auto plane = create();
	emplace<Position>(plane, glm::vec3(0.0f));
	emplace<Rotation>(plane, glm::vec3(0.0f));
	emplace<Scale>(plane, glm::vec3(1.0f));
	emplace<SceneObject>(plane, new PlanePrimitive(), Material(program, diffuse, specular));
}

void Registry::spawnMap(uint32_t _w, uint32_t _h)
{
	//bgfx::ProgramHandle program = loadProgram(&BxSingleton::getInstance().reader, "vs_blinnphong", "fs_blinnphong");

	std::pair<ProgramCache::iterator, bool> blinnphongProgram = PROGRAM_CACHE.load("blinnphong"_hs, "vs_blinnphong", "fs_blinnphong");
	std::pair<TextureCache::iterator, bool> loadedDiffuseTexture = TEXTURE_CACHE.load("texture_diffuse"_hs, "assets/textures/TreeTop_COLOR.png");
	std::pair<TextureCache::iterator, bool> loadedSpecularTexture = TEXTURE_CACHE.load("texture_specular"_hs, "assets/textures/TreeTop_SPEC.png");

	entt::resource<bgfx::ProgramHandle> program = blinnphongProgram.first->second;
	entt::resource<bgfx::TextureHandle> diffuse = loadedDiffuseTexture.first->second;
	entt::resource<bgfx::TextureHandle> specular = loadedSpecularTexture.first->second;


	Material m(program, diffuse, specular);

	for (uint32_t i = 0; i < _w; ++i) {
		for (uint32_t i = 0; i < _h; ++i) {
			auto tile = create();
			emplace<Position>(tile, glm::vec3(0.0f));
			emplace<Rotation>(tile, glm::vec3(0.0f));
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
