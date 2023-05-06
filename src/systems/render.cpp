#include "render.hpp"
#include <components/rotation.hpp>
#include <components/scale.hpp>
#include <components/position.hpp>
#include <components/scene_object.hpp>

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <bgfx/bgfx.h>


void renderScenObjects(Registry& registry)
{
	uint64_t state = 0
		| BGFX_STATE_WRITE_R
		| BGFX_STATE_WRITE_G
		| BGFX_STATE_WRITE_B
		| BGFX_STATE_WRITE_A
		| BGFX_STATE_WRITE_Z
		| BGFX_STATE_DEPTH_TEST_LESS
		| BGFX_STATE_CULL_CW
		| BGFX_STATE_MSAA
		;

	for (auto&& [entity, pos, rot, scale, obj] : registry.group<Position, Rotation, Scale, SceneObject>().each()) { 
		auto model = glm::mat4(1.0f);
		model = glm::translate(model, pos.position);
		model = glm::rotate(model, rot.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, rot.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, rot.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, scale.scale);

		// Set model matrix for rendering.
		bgfx::setTransform(glm::value_ptr(model));

		// Set render states.
		bgfx::setState(state);

		obj.primitive->submitPrimitive(obj.material);
	}
}
