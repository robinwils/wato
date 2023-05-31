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

using namespace entt::literals;

static void imageReleaseCb(void* _ptr, void* _userData)
{
	BX_UNUSED(_ptr);
	bimg::ImageContainer* imageContainer = (bimg::ImageContainer*)_userData;
	bimg::imageFree(imageContainer);
}


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
			unload(data);

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
	std::pair<TextureCache::iterator, bool> loadedDiffuseTexture = ResourceCache::instance().textureCache.load("texture"_hs, "assets/textures/TreeTop_COLOR.png");
	entt::resource<bgfx::TextureHandle> diffuse = loadedDiffuseTexture.first->second;

	bgfx::TextureHandle specular = loadTexture("assets/textures/TreeTop_SPEC.png");

	auto plane = create();
	emplace<Position>(plane, glm::vec3(0.0f));
	emplace<Rotation>(plane, glm::vec3(0.0f));
	emplace<Scale>(plane, glm::vec3(1.0f));
	emplace<SceneObject>(plane, new PlanePrimitive(), Material(program, diffuse, specular));
}
