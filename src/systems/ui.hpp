#pragma once

#include <entt/signal/dispatcher.hpp>

#include "core/menu/menu_events.hpp"
#include "core/net/pocketbase.hpp"
#include "systems/system.hpp"

/**
 * @brief Processes UI events
 *
 * Handles HTTP requests and matchmaking flow
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

    void onRegister(const RegisterEvent& aEvent);
    void onRegisterResult(const RegisterResultEvent& aEvent);

    void onJoinMatchmaking(const JoinMatchmakingEvent& aEvent);
    void onLeaveMatchmaking(const LeaveMatchmakingEvent& aEvent);
    void onJoinResult(const JoinResultEvent& aEvent);
    void onMatchFound(const MatchFoundEvent& aEvent);
    void onMatchmakingError(const MatchmakingErrorEvent& aEvent);

    bool mConnected = false;
};
