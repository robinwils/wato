#include "registry.hpp"
#include <glm/ext/vector_float3.hpp>
#include <components/position.hpp>
#include <components/rotation.hpp>
#include <components/scale.hpp>
#include <components/scene_object.hpp>
#include <renderer/plane.hpp>

void Registry::spawnPlane()
{
	//bgfx::ProgramHandle program = loadProgram(&BxFactory::getInstance().reader, "vs_cubes", "fs_cubes");
	bgfx::ProgramHandle program = loadProgram(&BxFactory::getInstance().reader, "vs_blinnphong", "fs_blinnphong");
	bgfx::TextureHandle diffuse = loadTexture("assets/textures/TreeTop_COLOR.png");
	bgfx::TextureHandle specular = loadTexture("assets/textures/TreeTop_SPEC.png");

	auto plane = create();
	emplace<Position>(plane, glm::vec3(0.0f));
	emplace<Rotation>(plane, glm::vec3(0.0f));
	emplace<Scale>(plane, glm::vec3(1.0f));
	emplace<SceneObject>(plane, new PlanePrimitive(), Material(program, diffuse, specular));
}
