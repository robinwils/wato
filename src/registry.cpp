#include "registry.hpp"
#include <glm/ext/vector_float3.hpp>
#include <components/position.hpp>
#include <components/rotation.hpp>
#include <components/scale.hpp>

void Registry::spawnPlane()
{
	auto plane = create();
	emplace<Position>(plane, glm::vec3(0.0f));
	emplace<Rotation>(plane, glm::vec3(0.0f));
	emplace<Scale>(plane, glm::vec3(4.0f));
}
