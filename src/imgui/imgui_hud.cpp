#include "imgui/imgui_hud.hpp"

#include <imgui.h>

#include "components/health.hpp"
#include "components/player.hpp"

void ImGuiHUD::Render(const Registry& aRegistry, const WatoWindow& aWin)
{
    auto width  = aWin.Width<float>();
    auto height = aWin.Height<float>();

    ImGui::SetNextWindowPos(ImVec2(10.0f, height - height / 3.5f - 10.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(width / 3.0f, height / 3.5f), ImGuiCond_FirstUseEver);
    ImGui::Begin("HUD");

    for (auto&& [entity, player, name, health] :
         aRegistry.view<Player, DisplayName, Health>().each()) {
        ImGui::Text("Player %s health: %f", name.Value.c_str(), health.Health);
    }
    ImGui::End();
}
