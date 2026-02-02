#pragma once

#include <entt/signal/dispatcher.hpp>

#include "core/menu/menu_events.hpp"
#include "core/net/network_events.hpp"
#include "core/net/pocketbase.hpp"
#include "systems/system.hpp"

/**
 * @brief Processes UI events
 *
 * Handles HTTP requests and more
 *
 */
class UISystem : public FrameSystem
{
   public:
    using FrameSystem::FrameSystem;

    const char* Name() const override { return "UISystem"; }

   protected:
    void Execute(Registry& aRegistry, float aTick) override;

   private:
    void ensureConnected(entt::dispatcher& aDispatcher);

    void onLogin(const LoginEvent& aEvent);
    void onLoginResult(const LoginResultEvent& aEvent);

    bool mConnected = false;
};
