#include "imgui/imgui_hud.hpp"

#include <imgui.h>

#include <chrono>

#include "components/game.hpp"
#include "components/health.hpp"
#include "components/player.hpp"
#include "imgui_helper.h"
#include "registry/registry.hpp"
#include "systems/system.hpp"

using namespace std::chrono_literals;

void ImGuiHUD::Render(const Registry& aRegistry, const WatoWindow& aWin)
{
    auto width  = aWin.Width<float>();
    auto height = aWin.Height<float>();

    const auto& game   = GetSingletonComponent<GameInstance>(aRegistry);
    const auto& defs   = GetSingletonComponent<const GameplayDef&>(aRegistry);
    const auto& pID    = GetSingletonComponent<PlayerID>(aRegistry);
    const auto& income = GetSingletonComponent<CommonIncome>(aRegistry);

    auto playerE = FindPlayerEntity(aRegistry, pID);
    auto redistributionTickInterval =
        std::chrono::duration_cast<GameTick>(defs.Economy.RedistributionInterval * 1s);
    long secs =
        std::chrono::duration_cast<std::chrono::seconds>(
            redistributionTickInterval - (game.Tick * GameTick(1) % redistributionTickInterval))
            .count();

    ImGui::SetNextWindowPos(ImVec2(10.0f, height - height / 3.5f - 10.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(width / 3.0f, height / 3.5f), ImGuiCond_FirstUseEver);
    ImGui::Begin("HUD");

    ImGui::TextColored(
        ImGui::Color::Gold,
        "Player %s",
        aRegistry.get<DisplayName>(playerE).Value.c_str());

    for (auto&& [entity, player, name, health] :
         aRegistry.view<Player, DisplayName, Health>().each()) {
        ImGui::Text("Player %s health: %f", name.Value.c_str(), health.Health);
    }
    ImGui::Text("Gold: %d", aRegistry.get<Gold>(playerE).Balance);
    ImGui::Text("Shared Income: %d", income.Value);
    ImGui::Text("Next Income in: %lds", secs);
    ImGui::End();
}
