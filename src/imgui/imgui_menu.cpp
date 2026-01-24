#include "imgui/imgui_menu.hpp"

#include <imgui.h>

#include <ranges>

#include "components/player.hpp"
#include "core/sys/log.hpp"
#include "registry/registry.hpp"

void ImGuiGameMenu::Render(const Registry& aRegistry, const WatoWindow& aWin)
{
    switch (mState) {
        case MenuState::MainMenu:
            renderMainMenu(aRegistry, aWin);
            break;
        case MenuState::InGame:
            renderInGame(aRegistry, aWin);
            break;
        case MenuState::EndGame:
            renderEndGame(aRegistry, aWin);
            break;
    }
}
void ImGuiGameMenu::renderMainMenu(const Registry& aRegistry, const WatoWindow& aWin) {}
void ImGuiGameMenu::renderInGame(const Registry& aRegistry, const WatoWindow& aWin) {}
void ImGuiGameMenu::renderEndGame(const Registry& aRegistry, const WatoWindow& aWin)
{
    auto& ranking = aRegistry.ctx().get<std::vector<PlayerID>>("ranking"_hs);

    // TODO: have imgui window creation helpers
    auto width  = aWin.Width<float>();
    auto height = aWin.Height<float>();

    float winW = width / 2.0f;
    float winH = height / 2.0f;

    ImGui::SetNextWindowPos(
        ImVec2(width - winW / 2.0f - winW, height - winH / 2.0f - winH),
        ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(winW, winH), ImGuiCond_FirstUseEver);
    ImGui::Begin("Final Board");

    std::ranges::reverse_view rv{ranking};
    for (auto i = 0U; i < rv.size(); ++i) {
        auto         rank   = i + 1;
        ImVec4       color  = ImColor(IM_COL32_WHITE);
        entt::entity player = FindPlayerEntity(aRegistry, rv[i]);

        switch (i) {
            case 0:
                color = ImColor(255, 215, 0);
                break;
            case 1:
                color = ImColor(192, 192, 192);
                break;
            case 2:
                color = ImColor(205, 127, 50);
                break;
            default:
                break;
        }
        ImGui::TextColored(color, "%d. %s", rank, aRegistry.get<Name>(player).Username.c_str());
    }
    ImGui::End();
}
