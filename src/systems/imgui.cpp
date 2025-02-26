#include "components/imgui.hpp"

#include <glm/gtc/type_ptr.hpp>

#include "components/camera.hpp"
#include "components/transform3d.hpp"
#include "core/registry.hpp"
#include "imgui_helper.h"
#include "input/input.hpp"

void renderImgui(Registry& registry, float width, float height)
{
    const auto& input = registry.ctx().get<Input&>();

    imguiBeginFrame(input, uint16_t(width), uint16_t(height));

    for (auto&& entity : registry.view<ImguiDrawable>()) {
        auto [camera, transform] = registry.try_get<Camera, Transform3D>(entity);
        if (camera && transform) {
            showImguiDialogs(input, *camera, width, height);
            ImGui::Text("Camera Setting");
            ImGui::DragFloat3("Position", glm::value_ptr(transform->position), 0.1f, 5.0f);
            ImGui::DragFloat3("Direction", glm::value_ptr(camera->dir), 0.1f, 2.0f);
            ImGui::DragFloat("FoV (Degree)", &camera->fov, 10.0f, 120.0f);
            ImGui::DragFloat("Speed", &camera->speed, 0.1f, 0.01f, 5.0f, "%.03f");
        }
    }

    imguiEndFrame();
}
