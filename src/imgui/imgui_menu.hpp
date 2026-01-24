#pragma once

#include "core/menu/menu_state.hpp"
#include "core/window.hpp"
#include "registry/registry.hpp"

class ImGuiGameMenu
{
   public:
    ImGuiGameMenu() : mState(MenuState::InGame) {}

    void GoInGame() { mState = MenuState::InGame; }
    void GoToMain() { mState = MenuState::MainMenu; }
    void GoToEnd() { mState = MenuState::EndGame; }

    void Render(const Registry& aRegistry, const WatoWindow& aWin);

   private:
    void renderMainMenu(const Registry& aRegistry, const WatoWindow& aWin);
    void renderInGame(const Registry& aRegistry, const WatoWindow& aWin);
    void renderEndGame(const Registry& aRegistry, const WatoWindow& aWin);

    MenuState mState;
};
