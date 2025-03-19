#include <bgfx/bgfx.h>

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "components/light_source.hpp"
#include "components/placement_mode.hpp"
#include "components/scene_object.hpp"
#include "components/transform3d.hpp"
#include "core/cache.hpp"
#include "systems.hpp"

void renderSceneObjects(Registry& aRegistry, const float /*dt*/)
{
    uint64_t state = BGFX_STATE_DEFAULT;

    auto bpShader = WATO_PROGRAM_CACHE["blinnphong"_hs];
    // light
    for (auto&& [light, source] : aRegistry.view<LightSource>().each()) {
        bgfx::setUniform(bpShader->Uniform("u_lightDir"),
            glm::value_ptr(glm::vec4(source.direction, 0.0f)));
        bgfx::setUniform(bpShader->Uniform("u_lightCol"),
            glm::value_ptr(glm::vec4(source.color, 0.0f)));
    }
    auto group = aRegistry.group<SceneObject>(entt::get<Transform3D>);
    auto check = aRegistry.view<PlacementMode>();

    for (auto&& [entity, obj, t] : group.each()) {
        auto model = glm::mat4(1.0f);
        model      = glm::translate(model, t.Position);
        model      = glm::rotate(model, glm::radians(t.Rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model      = glm::rotate(model, glm::radians(t.Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model      = glm::rotate(model, glm::radians(t.Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        model      = glm::scale(model, t.Scale);

        if (check.contains(entity)) {
            // DBG("GOT Placement mode entity!")
        }

        if (auto primitives = WATO_MODEL_CACHE[obj.model_hash]; primitives) {
            for (const auto* p : *primitives) {
                // Set model matrix for rendering.
                bgfx::setTransform(glm::value_ptr(model));

                // kinda awkward place to put that...
                // obj.material.drawImgui();

                // Set render states.
                bgfx::setState(state);
                p->Submit();
            }
        }
    }
}
