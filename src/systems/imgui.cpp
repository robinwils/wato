#include "components/imgui.hpp"

#include <glm/gtc/type_ptr.hpp>

#include "components/camera.hpp"
#include "components/light_source.hpp"
#include "components/physics.hpp"
#include "components/transform3d.hpp"
#include "config.h"
#include "core/registry.hpp"
#include "glm/gtx/string_cast.hpp"
#include "imgui.h"
#include "imgui_helper.h"
#include "input/input.hpp"
#include "renderer/physics.hpp"

void renderImgui(Registry& registry, float width, float height)
{
    const auto& input = registry.ctx().get<Input&>();

    imguiBeginFrame(input, uint16_t(width), uint16_t(height));
    showImguiDialogs(width, height);

    for (auto&& [entity, imgui] : registry.view<ImguiDrawable>().each()) {
        auto [camera, transform] = registry.try_get<Camera, Transform3D>(entity);
        ImGui::Text("%s Settings", imgui.name.c_str());
        if (camera && transform) {
            input.drawImgui(*camera, transform->position, width, height);
            ImGui::DragFloat3("Position", glm::value_ptr(transform->position), 0.1f, 5.0f);
            ImGui::DragFloat3("Direction", glm::value_ptr(camera->dir), 0.1f, 2.0f);
            ImGui::DragFloat("FoV (Degree)", &camera->fov, 10.0f, 120.0f);
            ImGui::DragFloat("Speed", &camera->speed, 0.1f, 0.01f, 5.0f, "%.03f");
            continue;
        }
        if (transform) {
            ImGui::Text("Position = %s", glm::to_string(transform->position).c_str());
            continue;
        }

        auto* light_source = registry.try_get<LightSource>(entity);
        if (light_source) {
            ImGui::DragFloat3("Light direction",
                glm::value_ptr(light_source->direction),
                0.1f,
                5.0f);
            ImGui::DragFloat3("Light color", glm::value_ptr(light_source->color), 0.10f, 2.0f);
        }
    }

    auto& params = registry.ctx().get<PhysicsParams>();
    auto& phy    = registry.ctx().get<Physics>();
    auto* logger = phy.common.getLogger();

    ImGui::Text("Physics info");
    if (ImGui::Checkbox("Information Logs", &params.info_logs)
        || ImGui::Checkbox("Warning Logs", &params.warning_logs)
        || ImGui::Checkbox("Error logs", &params.error_logs)) {
        uint log_level = 0;
        if (params.info_logs) {
            log_level |= static_cast<uint>(rp3d::Logger::Level::Information);
        }
        if (params.warning_logs) {
            log_level |= static_cast<uint>(rp3d::Logger::Level::Warning);
        }
        if (params.error_logs) {
            log_level |= static_cast<uint>(rp3d::Logger::Level::Error);
        }
        params.logger->removeAllDestinations();
        params.logger->addStreamDestination(std::cout,
            log_level,
            rp3d::DefaultLogger::Format::Text);
    }

#if WATO_DEBUG
    rp3d::DebugRenderer& debug_renderer = phy.world->getDebugRenderer();
    auto                 n_tri          = debug_renderer.getNbTriangles();
    auto                 n_lines        = debug_renderer.getNbLines();

    ImGui::Text("%d debug lines and %d debug triangles", n_lines, n_tri);
    ImGui::Checkbox("Collider Shapes", &params.render_shapes);
    ImGui::Checkbox("Collider AABB", &params.render_aabb);

    debug_renderer.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::COLLISION_SHAPE,
        params.render_shapes);
    debug_renderer.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::COLLIDER_AABB,
        params.render_aabb);
    debug_renderer.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::CONTACT_POINT,
        params.render_contact_points);
    debug_renderer.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::CONTACT_NORMAL,
        params.render_contact_normals);
#endif

    imguiEndFrame();
}
