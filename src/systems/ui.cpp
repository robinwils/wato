#include "ui.hpp"

#include "core/net/pocketbase.hpp"
#include "core/sys/log.hpp"

using namespace entt::literals;

void UISystem::ensureConnected(entt::dispatcher& aDispatcher)
{
    if (mConnected) {
        return;
    }

    aDispatcher.sink<LoginEvent>().connect<&UISystem::onLogin>(*this);
    aDispatcher.sink<LoginResultEvent>().connect<&UISystem::onLoginResult>(*this);

    mConnected = true;
}

void UISystem::Execute(Registry& aRegistry, [[maybe_unused]] float aTick)
{
    auto& dispatcher = aRegistry.ctx().get<entt::dispatcher>("ui_dispatcher"_hs);

    ensureConnected(dispatcher);
    dispatcher.update();
}

void UISystem::onLogin(const LoginEvent& aEvent)
{
    Registry* reg        = aEvent.Reg;
    auto&     pb         = reg->ctx().get<PocketBaseClient>();
    auto&     dispatcher = reg->ctx().get<entt::dispatcher>("ui_dispatcher"_hs);

    pb.LoginCtx.State = LoginState::Pending;

    pb.Auth.Login(
        aEvent.Account,
        aEvent.Password,
        [reg, &dispatcher](
            const std::optional<AuthService::LoginResult>& aResult, const std::string& aError) {
            dispatcher.enqueue<LoginResultEvent>(
                LoginResultEvent{.Reg = reg, .Result = aResult, .Error = aError});
        });
}

void UISystem::onLoginResult(const LoginResultEvent& aEvent)
{
    Registry& registry = *aEvent.Reg;
    auto&     pb       = registry.ctx().get<PocketBaseClient>();

    if (aEvent.Result) {
        pb.LoginCtx.State  = LoginState::Success;
        pb.LoginCtx.Result = aEvent.Result;
        pb.SetToken(aEvent.Result->token);
        WATO_INFO(registry, "user {} logged in", aEvent.Result->record.accountName);
    } else {
        pb.LoginCtx.State = LoginState::Failed;
        pb.LoginCtx.Error = aEvent.Error;
        WATO_ERR(registry, "login failed: {}", aEvent.Error);
    }
}
