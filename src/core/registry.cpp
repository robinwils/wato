#include "registry.hpp"
#include <glm/ext/vector_float3.hpp>
#include <components/position.hpp>
#include <components/rotation.hpp>
#include <components/scale.hpp>
#include <components/scene_object.hpp>
#include <renderer/plane.hpp>

void Registry::spawnPlane()
{
	bgfx::ProgramHandle program = loadProgram(m_bxFactory.getDefaultFileReader(), "vs_cubes", "fs_cubes");

	auto plane = create();
	emplace<Position>(plane, glm::vec3(0.0f));
	emplace<Rotation>(plane, glm::vec3(0.0f));
	emplace<Scale>(plane, glm::vec3(4.0f));
	emplace<SceneObject>(plane, new PlanePrimitive(), Material());
}
