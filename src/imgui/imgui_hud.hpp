#pragma once

#include "core/hud.hpp"
#include "core/window.hpp"
#include "registry/registry.hpp"

class ImGuiHUD : public HeadsUpDisplay
{
   public:
    ImGuiHUD() {}
    virtual ~ImGuiHUD() {}

    virtual void Render(const Registry& aRegistry, const WatoWindow& aWin);
};
