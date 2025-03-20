#include "components/imgui.hpp"

#include <glm/gtc/type_ptr.hpp>

#include "components/light_source.hpp"
#include "components/transform3d.hpp"
#include "imgui.h"
#include "imgui_helper.h"
#include "renderer/physics.hpp"
#include "systems/systems.hpp"

void renderImgui(Registry& aRegistry, WatoWindow& aWin)
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

    auto& params = aRegistry.ctx().get<PhysicsParams>();
    auto& phy    = aRegistry.ctx().get<Physics>();

    ImGui::Text("Physics info");
    if (ImGui::Checkbox("Information Logs", &params.InfoLogs)
        || ImGui::Checkbox("Warning Logs", &params.WarningLogs)
        || ImGui::Checkbox("Error logs", &params.ErrorLogs)) {
        uint logLevel = 0;
        if (params.InfoLogs) {
            logLevel |= static_cast<uint>(rp3d::Logger::Level::Information);
        }
        if (params.WarningLogs) {
            logLevel |= static_cast<uint>(rp3d::Logger::Level::Warning);
        }
        if (params.ErrorLogs) {
            logLevel |= static_cast<uint>(rp3d::Logger::Level::Error);
        }
        params.Logger->removeAllDestinations();
        params.Logger->addStreamDestination(std::cout, logLevel, rp3d::DefaultLogger::Format::Text);
    }

#if WATO_DEBUG
    rp3d::DebugRenderer& debugRenderer = phy.World->getDebugRenderer();
    auto                 nTri          = debugRenderer.getNbTriangles();
    auto                 nLines        = debugRenderer.getNbLines();

    ImGui::Text("%d debug lines and %d debug triangles", nLines, nTri);
    ImGui::Checkbox("Collider Shapes", &params.RenderShapes);
    ImGui::Checkbox("Collider AABB", &params.RenderAabb);

    debugRenderer.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::COLLISION_SHAPE,
        params.RenderShapes);
    debugRenderer.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::COLLIDER_AABB,
        params.RenderAabb);
    debugRenderer.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::CONTACT_POINT,
        params.RenderContactPoints);
    debugRenderer.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::CONTACT_NORMAL,
        params.RenderContactNormals);
#endif

    imguiEndFrame();
}
