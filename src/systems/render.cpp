#include "systems/render.hpp"

#include <bgfx/bgfx.h>

#include <cstdint>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "components/imgui.hpp"
#include "components/light_source.hpp"
#include "components/physics.hpp"
#include "components/placement_mode.hpp"
#include "components/scene_object.hpp"
#include "components/transform3d.hpp"
#include "core/cache.hpp"
#include "glm/ext/quaternion_transform.hpp"
#include "imgui_helper.h"

void RenderSystem::operator()(Registry& aRegistry)
{
    // This dummy draw call is here to make sure that view 0 is cleared
    // if no other draw calls are submitted to view 0.
    bgfx::touch(0);

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
        auto model  = glm::identity<glm::mat4>();
        model       = glm::translate(model, t.Position);
        model      *= glm::mat4_cast(t.Orientation);
        model       = glm::scale(model, t.Scale);

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

void RenderImguiSystem::operator()(Registry& aRegistry, const float aDeltaTime, WatoWindow& aWin)
{
    imguiBeginFrame(aWin.GetInput(), aWin.Width<int>(), aWin.Height<int>());
    showImguiDialogs(aWin.Width<float>(), aWin.Height<float>());

    for (auto&& [entity, imgui] : aRegistry.view<ImguiDrawable>().each()) {
        auto [camera, transform] = aRegistry.try_get<Camera, Transform3D>(entity);
        ImGui::Text("%s Settings", imgui.name.c_str());
        if (camera && transform) {
            aWin.GetInput().DrawImgui(*camera,
                transform->Position,
                aWin.Width<float>(),
                aWin.Height<float>());
            ImGui::DragFloat3("Position", glm::value_ptr(transform->Position), 0.1f, 5.0f);
            ImGui::DragFloat3("Direction", glm::value_ptr(camera->Dir), 0.1f, 2.0f);
            ImGui::DragFloat("FoV (Degree)", &camera->Fov, 10.0f, 120.0f);
            ImGui::DragFloat("Speed", &camera->Speed, 0.1f, 0.01f, 5.0f, "%.03f");
            continue;
        }
        if (transform) {
            ImGui::Text("Position = %s", glm::to_string(transform->Position).c_str());
            continue;
        }

        auto* lightSource = aRegistry.try_get<LightSource>(entity);
        if (lightSource) {
            ImGui::DragFloat3("Light direction",
                glm::value_ptr(lightSource->direction),
                0.1f,
                5.0f);
            ImGui::DragFloat3("Light color", glm::value_ptr(lightSource->color), 0.10f, 2.0f);
        }
    }

    auto& phy = aRegistry.GetPhysics();

    ImGui::Text("Physics info");
    if (ImGui::Checkbox("Information Logs", &phy.InfoLogs)
        || ImGui::Checkbox("Warning Logs", &phy.WarningLogs)
        || ImGui::Checkbox("Error logs", &phy.ErrorLogs)) {
        uint logLevel = 0;
        if (phy.InfoLogs) {
            logLevel |= static_cast<uint>(rp3d::Logger::Level::Information);
        }
        if (phy.WarningLogs) {
            logLevel |= static_cast<uint>(rp3d::Logger::Level::Warning);
        }
        if (phy.ErrorLogs) {
            logLevel |= static_cast<uint>(rp3d::Logger::Level::Error);
        }
        phy.Logger->removeAllDestinations();
        phy.Logger->addStreamDestination(std::cout, logLevel, rp3d::DefaultLogger::Format::Text);
    }

#if WATO_DEBUG
    rp3d::DebugRenderer& debugRenderer = phy.World->getDebugRenderer();
    auto                 nTri          = debugRenderer.getNbTriangles();
    auto                 nLines        = debugRenderer.getNbLines();

    ImGui::Text("%d debug lines and %d debug triangles", nLines, nTri);
    ImGui::Checkbox("Collider Shapes", &phy.RenderShapes);
    ImGui::Checkbox("Collider AABB", &phy.RenderAabb);

    debugRenderer.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::COLLISION_SHAPE,
        phy.RenderShapes);
    debugRenderer.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::COLLIDER_AABB,
        phy.RenderAabb);
    debugRenderer.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::CONTACT_POINT,
        phy.RenderContactPoints);
    debugRenderer.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::CONTACT_NORMAL,
        phy.RenderContactNormals);
#endif

    imguiEndFrame();
}

void CameraSystem::operator()(Registry& aRegistry, const float aDeltaTime, WatoWindow& aWin)
{
    for (auto&& [entity, camera, transform] : aRegistry.view<Camera, Transform3D>().each()) {
        const auto& viewMat = camera.View(transform.Position);
        const auto& proj    = camera.Projection(aWin.Width<float>(), aWin.Height<float>());
        bgfx::setViewTransform(0, glm::value_ptr(viewMat), glm::value_ptr(proj));
        bgfx::setViewRect(0, 0, 0, aWin.Width<uint16_t>(), aWin.Height<uint16_t>());

        // just because I know there is only 1 camera (for now)
        // TODO: put in registry context var as singleton ?
        break;
    }
}
