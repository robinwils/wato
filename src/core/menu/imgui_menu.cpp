#include "core/menu/imgui_menu.hpp"

#include <cstring>
#include <ranges>

#include "components/player.hpp"
#include "core/menu/menu.hpp"
#include "core/menu/menu_events.hpp"
#include "core/net/pocketbase.hpp"
#include "core/window.hpp"
#include "imgui/imgui_helper.h"
#include "registry/registry.hpp"

void ImGuiMenu::Render(Registry& aReg)
{
    switch (aReg.ctx().get<MenuContext>().State) {
        case MenuState::Login:
        case MenuState::Register:
        case MenuState::Lobby:
            renderMainMenu(aReg);
            break;
        case MenuState::InGame:
            renderInGame(aReg);
            break;
        case MenuState::EndGame:
            renderEndGame(aReg);
            break;
    }
}

void ImGuiMenu::renderMainMenu(Registry& aRegistry)
{
    auto& win        = GetSingletonComponent<WatoWindow>(aRegistry);
    auto& menu       = GetSingletonComponent<MenuContext>(aRegistry);
    auto& dispatcher = menu.Dispatcher;

    auto  width  = win.Width<float>();
    auto  height = win.Height<float>();
    float winW   = width * 0.2f;
    float winH   = height * 0.2f;

    ImGui::SetNextWindowPos(
        ImVec2(width * 0.5f - winW / 2.0f, height * 0.5f - winH * 0.5f),
        ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(winW, winH), ImGuiCond_FirstUseEver);

    ImGui::Begin("Login");

    switch (menu.State) {
        break;
        case MenuState::Login:
            renderLogin(aRegistry);
            break;
        case MenuState::Register:
            renderRegister(aRegistry);
            break;
        case MenuState::Lobby:
            renderLobby(aRegistry);
            break;
        default:
            break;
    }

    ImGui::End();
}

void ImGuiMenu::renderLogin(Registry& aRegistry)
{
    auto& menu       = GetSingletonComponent<MenuContext>(aRegistry);
    auto& dispatcher = menu.Dispatcher;

    static char account[64]  = {0};
    static char password[64] = {0};
    ImGui::InputTextWithHint("##account", "<Account#TAG>", account, IM_ARRAYSIZE(account));
    ImGui::InputTextWithHint(
        "##password",
        "<Password>",
        password,
        IM_ARRAYSIZE(password),
        ImGuiInputTextFlags_Password);
    ImGui::SameLine();
    ImGui::HelpMarker(
        "Display all characters as '*'.\nDisable clipboard cut and copy.\nDisable logging.\n");

    bool isPending = menu.LoginState == LoginState::Pending;
    if (isPending) {
        menu.Message = "Logging in...";
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("Login")) {
        dispatcher.enqueue(LoginEvent{
            .Reg      = &aRegistry,
            .Account  = std::move(account),
            .Password = std::move(password)});
    }

    if (ImGui::Button("Register")) {
        menu.State = MenuState::Register;
        menu.ClearMsgs();
    }

    if (isPending) {
        ImGui::EndDisabled();
    }

    renderStatusMsg(aRegistry);
}

void ImGuiMenu::renderRegister(Registry& aRegistry)
{
    auto& menu       = GetSingletonComponent<MenuContext>(aRegistry);
    auto& dispatcher = menu.Dispatcher;

    static char account[64]        = {0};
    static char tag[4]             = {0};
    static char password[64]       = {0};
    static char passwordRepeat[64] = {0};
    ImGui::InputTextWithHint("##account", "<Account>", account, IM_ARRAYSIZE(account));
    ImGui::InputTextWithHint("##tag", "<Tag>", tag, IM_ARRAYSIZE(tag));

    ImGui::InputTextWithHint(
        "##password",
        "<Password>",
        password,
        IM_ARRAYSIZE(password),
        ImGuiInputTextFlags_Password);
    ImGui::SameLine();
    ImGui::HelpMarker(
        "Display all characters as '*'.\nDisable clipboard cut and copy.\nDisable logging.\n");

    ImGui::InputTextWithHint(
        "##passwordrep",
        "<Repeat Password>",
        passwordRepeat,
        IM_ARRAYSIZE(passwordRepeat),
        ImGuiInputTextFlags_Password);

    bool isPending      = menu.RegisterState == RegisterState::Pending;
    bool passwordsMatch = 0 == strncmp(password, passwordRepeat, 64);

    if (isPending || !passwordsMatch) {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("Register") && passwordsMatch) {
        dispatcher.enqueue(RegisterEvent{
            .Reg         = &aRegistry,
            .AccountName = fmt::format("{}#{}", account, tag),
            .Password    = std::move(password),
        });
    }

    if (!passwordsMatch) {
        menu.Error = "Passwords don't match";
        ImGui::EndDisabled();
    }

    if (ImGui::Button("Login")) {
        menu.State = MenuState::Login;
        menu.ClearMsgs();
    }

    if (isPending) {
        menu.Message = "Registering...";
        ImGui::EndDisabled();
    }

    renderStatusMsg(aRegistry);
}

void ImGuiMenu::renderStatusMsg(Registry& aRegistry)
{
    auto& menu = GetSingletonComponent<MenuContext>(aRegistry);

    if (!menu.Message.empty()) {
        ImGui::Text("%s", menu.Message.c_str());
    }
    if (!menu.Error.empty()) {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", menu.Error.c_str());
    }
}

void ImGuiMenu::renderLobby(Registry& aRegistry)
{
    auto& menu       = GetSingletonComponent<MenuContext>(aRegistry);
    auto& dispatcher = menu.Dispatcher;
    auto& pb         = GetSingletonComponent<PocketBaseClient>(aRegistry);

    ImGui::Text("Hello %s!", pb.LoggedUser.accountName.c_str());
    ImGui::Separator();

    switch (menu.Matchmaking.State) {
        case MatchmakingState::Failed:
        case MatchmakingState::Idle:
            if (ImGui::Button("Find Match")) {
                dispatcher.enqueue(JoinMatchmakingEvent{
                    .Reg = &aRegistry,
                });
            }
            break;
        case MatchmakingState::Joining:
        case MatchmakingState::Waiting:
            menu.Message = "Waiting for match...";
            if (ImGui::Button("Cancel")) {
                dispatcher.enqueue(JoinMatchmakingEvent{
                    .Reg = &aRegistry,
                });
            }
            break;
        case MatchmakingState::Matched:
        case MatchmakingState::Connecting:
            break;
    }

    renderStatusMsg(aRegistry);
}

void ImGuiMenu::renderInGame(const Registry& aRegistry) {}
void ImGuiMenu::renderEndGame(const Registry& aRegistry)
{
    auto& ranking = aRegistry.ctx().get<std::vector<PlayerID>>("ranking"_hs);
    auto& win     = GetSingletonComponent<WatoWindow>(aRegistry);

    // TODO: have imgui window creation helpers
    auto  width  = win.Width<float>();
    auto  height = win.Height<float>();
    float winW   = width / 2.0f;
    float winH   = height / 2.0f;

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
                // gold
                color = ImColor(255, 215, 0);
                break;
            case 1:
                // silver
                color = ImColor(192, 192, 192);
                break;
            case 2:
                // bronze
                color = ImColor(205, 127, 50);
                break;
            default:
                break;
        }
        ImGui::TextColored(
            color,
            "%d. %s",
            rank,
            aRegistry.get<::DisplayName>(player).Value.c_str());
    }
    ImGui::End();
}
