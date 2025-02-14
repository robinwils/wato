#include "render.hpp"

#include <bgfx/bgfx.h>

#include <components/color.hpp>
#include <components/direction.hpp>
#include <components/position.hpp>
#include <components/rotation.hpp>
#include <components/scale.hpp>
#include <components/scene_object.hpp>
#include <core/cache.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

void renderSceneObjects(Registry& registry, const float dt)
{
    uint64_t state = 0 | BGFX_STATE_WRITE_R | BGFX_STATE_WRITE_G | BGFX_STATE_WRITE_B | BGFX_STATE_WRITE_A
                     | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_CULL_CW | BGFX_STATE_MSAA;

    // light
    auto view        = registry.view<Direction, Color>();
    auto lightEntity = view.front();
    assert(lightEntity != entt::null);
    // bgfx::setUniform(registry.get<Direction, glm::value_ptr(glm::vec4(m_lightDir, 0.0f)));
    // bgfx::setUniform(u_lightCol, glm::value_ptr(glm::vec4(m_lightCol, 0.0f)));

    for (auto&& [entity, pos, rot, scale, obj] : registry.view<Position, Rotation, Scale, SceneObject>().each()) {
        auto model = glm::mat4(1.0f);
        model      = glm::translate(model, pos.position);
        model      = glm::rotate(model, dt * rot.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        model      = glm::rotate(model, dt * rot.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        model      = glm::rotate(model, dt * rot.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
        model      = glm::scale(model, scale.scale);

        for (const auto* p : *MODEL_CACHE[obj.model_hash]) {
            // Set model matrix for rendering.
            bgfx::setTransform(glm::value_ptr(model));

        // kinda awkward place to put that...
        obj.material.drawImgui();

            // Set render states.
            bgfx::setState(state);
            p->submit();
        }
    }
}
