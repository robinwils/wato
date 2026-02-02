#include "ui.hpp"

#include "core/menu/menu.hpp"
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
            const std::optional<AuthService::LoginResult>& aResult,
            const std::string&                             aError) {
            dispatcher.enqueue<LoginResultEvent>(LoginResultEvent{
                .Reg         = reg,
                .Avatar      = aResult->record.avatar,
                .Email       = aResult->record.email,
                .AccountName = aResult->record.accountName,
                .Token       = aResult->token,
                .Error       = aError});
        });
}

void UISystem::onLoginResult(const LoginResultEvent& aEvent)
{
    Registry& registry = *aEvent.Reg;

    auto& pb   = registry.ctx().get<PocketBaseClient>();
    auto& menu = registry.ctx().get<MenuContext>();

    if (aEvent.Error.empty()) {
        menu.LoginState = LoginState::Success;
        pb.Token        = aEvent.Token;

        WATO_INFO(registry, "user {} logged in", aEvent.AccountName);
    } else {
        menu.LoginState = LoginState::Failed;
        menu.LoginError = aEvent.Error;

        WATO_ERR(registry, "login failed: {}", aEvent.Error);
    }
}
