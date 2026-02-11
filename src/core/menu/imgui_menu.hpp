#pragma once

#include "core/menu/menu_backend.hpp"

class ImGuiMenu : public MenuBackend
{
   public:
    virtual ~ImGuiMenu() {}
    virtual void Render(Registry& aReg);

   private:
    void renderMainMenu(Registry& aRegistry);
    void renderLogin(Registry& aRegistry);
    void renderRegister(Registry& aRegistry);
    void renderLobby(Registry& aRegistry);
    void renderStatusMsg(Registry& aRegistry);

    void renderInGame(const Registry& aRegistry);
    void renderEndGame(const Registry& aRegistry);
};
