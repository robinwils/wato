#include <bgfx/bgfx.h>

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "components/color.hpp"
#include "components/direction.hpp"
#include "components/placement_mode.hpp"
#include "components/scene_object.hpp"
#include "components/transform3d.hpp"
#include "core/cache.hpp"
#include "systems.hpp"

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
    auto group = registry.group<SceneObject>(entt::get<Transform3D>);
    auto check = registry.view<PlacementMode>();

    for (auto&& [entity, obj, t] : group.each()) {
        auto model = glm::mat4(1.0f);
        model      = glm::translate(model, t.position);
        model      = glm::rotate(model, glm::radians(t.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model      = glm::rotate(model, glm::radians(t.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model      = glm::rotate(model, glm::radians(t.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        model      = glm::scale(model, t.scale);

        if (check.contains(entity)) {
            // DBG("GOT Placement mode entity!")
        }

        if (auto primitives = MODEL_CACHE[obj.model_hash]; primitives) {
            for (const auto* p : *primitives) {
                // Set model matrix for rendering.
                bgfx::setTransform(glm::value_ptr(model));

                // kinda awkward place to put that...
                // obj.material.drawImgui();

                // Set render states.
                bgfx::setState(state);
                p->submit();
            }
        }
    }
}
