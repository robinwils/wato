#include "ui.hpp"

#include <expected>

#include "core/menu/menu.hpp"
#include "core/net/enet_client.hpp"
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
    aDispatcher.sink<RegisterEvent>().connect<&UISystem::onRegister>(*this);
    aDispatcher.sink<RegisterResultEvent>().connect<&UISystem::onRegisterResult>(*this);
    aDispatcher.sink<JoinMatchmakingEvent>().connect<&UISystem::onJoinMatchmaking>(*this);
    aDispatcher.sink<LeaveMatchmakingEvent>().connect<&UISystem::onLeaveMatchmaking>(*this);
    aDispatcher.sink<JoinResultEvent>().connect<&UISystem::onJoinResult>(*this);
    aDispatcher.sink<MatchmakingErrorEvent>().connect<&UISystem::onMatchmakingError>(*this);

    mConnected = true;
}

void UISystem::Execute(Registry& aRegistry, [[maybe_unused]] float aTick)
{
    auto& menu = aRegistry.ctx().get<MenuContext>();

    ensureConnected(menu.Dispatcher);
    menu.Dispatcher.update();
}

void UISystem::onLogin(const LoginEvent& aEvent)
{
    Registry* reg  = aEvent.Reg;
    auto&     pb   = reg->ctx().get<PocketBaseClient>();
    auto&     menu = reg->ctx().get<MenuContext>();

    menu.LoginState = LoginState::Pending;

    pb.Login(
        aEvent.Account,
        aEvent.Password,
        [reg](std::expected<LoginResult, PBError> aResult) {
            auto& dispatcher = reg->ctx().get<MenuContext>().Dispatcher;
            if (aResult) {
                dispatcher.enqueue<LoginResultEvent>(LoginResultEvent{
                    .Reg         = reg,
                    .ID          = aResult->record.id,
                    .Avatar      = aResult->record.avatar,
                    .Email       = aResult->record.email,
                    .AccountName = aResult->record.accountName,
                    .Token       = aResult->token,
                    .Error       = ""});
            } else {
                dispatcher.enqueue<LoginResultEvent>(
                    LoginResultEvent{.Reg = reg, .Error = aResult.error().Message});
            }
        });

    WATO_INFO(*reg, "pb.Login() returned (not blocked)");
}

void UISystem::onLoginResult(const LoginResultEvent& aEvent)
{
    Registry& registry = *aEvent.Reg;

    auto& pb   = registry.ctx().get<PocketBaseClient>();
    auto& menu = registry.ctx().get<MenuContext>();

    if (aEvent.Error.empty()) {
        auto playerID = PlayerIDFromHexString(aEvent.ID);
        if (!playerID) {
            WATO_ERR(registry, "invalid player ID: '{}'", aEvent.ID);
            return;
        }

        menu.LoginState = LoginState::Success;
        menu.State      = MenuState::Lobby;
        pb.Token        = aEvent.Token;
        pb.LoggedUser   = LoginRecord{
              .id          = aEvent.ID,
              .avatar      = aEvent.Avatar,
              .email       = aEvent.Email,
              .accountName = aEvent.AccountName,
        };
        menu.ClearMsgs();

        auto& netClient = registry.ctx().get<ENetClient&>();
        netClient.Connect();

        WATO_INFO(registry, "user {} logged in", aEvent.AccountName);
    } else {
        menu.LoginState = LoginState::Failed;
        menu.Error      = aEvent.Error;
        menu.Message.clear();

        WATO_ERR(registry, "login failed: {}", aEvent.Error);
    }
}

void UISystem::onRegister(const RegisterEvent& aEvent)
{
    Registry* reg  = aEvent.Reg;
    auto&     pb   = reg->ctx().get<PocketBaseClient>();
    auto&     menu = reg->ctx().get<MenuContext>();

    menu.RegisterState = RegisterState::Pending;

    pb.Register(
        aEvent.AccountName,
        aEvent.Password,
        [reg](std::expected<RegisterResult, PBError> aResult) {
            auto& dispatcher = reg->ctx().get<MenuContext>().Dispatcher;
            if (aResult) {
                dispatcher.enqueue<RegisterResultEvent>(RegisterResultEvent{
                    .Reg         = reg,
                    .ID          = aResult->id,
                    .AccountName = aResult->accountName,
                    .Error       = ""});
            } else {
                dispatcher.enqueue<RegisterResultEvent>(
                    RegisterResultEvent{.Reg = reg, .Error = aResult.error().Message});
            }
        });
}

void UISystem::onRegisterResult(const RegisterResultEvent& aEvent)
{
    Registry& registry = *aEvent.Reg;

    auto& menu = registry.ctx().get<MenuContext>();

    if (aEvent.Error.empty()) {
        menu.RegisterState = RegisterState::Success;
        menu.Message       = fmt::format("user {} registered, please log in.", aEvent.AccountName);
        menu.Error.clear();
        menu.State = MenuState::Login;

        WATO_INFO(registry, "user {} registered", aEvent.AccountName);
    } else {
        menu.RegisterState = RegisterState::Failed;
        menu.Error         = aEvent.Error;
        menu.Message.clear();

        WATO_ERR(registry, "register failed: {}", aEvent.Error);
    }
}

void UISystem::onJoinMatchmaking(const JoinMatchmakingEvent& aEvent)
{
    Registry* reg  = aEvent.Reg;
    auto&     pb   = reg->ctx().get<PocketBaseClient>();
    auto&     menu = reg->ctx().get<MenuContext>();

    if (menu.Matchmaking.State != MatchmakingState::Idle
        && menu.Matchmaking.State != MatchmakingState::Failed) {
        return;
    }

    menu.Matchmaking.State = MatchmakingState::Joining;
    menu.ClearMsgs();

    pb.JoinQueue(
        pb.LoggedUser.id,
        1,
        2,
        [reg](std::expected<MatchmakingRecord, PBError> aResult) {
            auto& dispatcher = reg->ctx().get<MenuContext>().Dispatcher;
            if (aResult) {
                dispatcher.enqueue(JoinResultEvent{
                    .Reg         = reg,
                    .ID          = aResult->id,
                    .AccountName = aResult->accountName,
                    .Status      = aResult->status,
                    .ServerAddr  = aResult->serverAddr,
                    .Created     = aResult->created,
                    .Updated     = aResult->updated,
                });
            } else {
                dispatcher.enqueue<MatchmakingErrorEvent>(
                    MatchmakingErrorEvent{.Reg = reg, .Error = aResult.error().Message});
            }
        });
}

void UISystem::onLeaveMatchmaking(const LeaveMatchmakingEvent& aEvent)
{
    Registry* reg = aEvent.Reg;

    auto& pb   = reg->ctx().get<PocketBaseClient>();
    auto& menu = reg->ctx().get<MenuContext>();

    if (menu.Matchmaking.State != MatchmakingState::Joining
        && menu.Matchmaking.State != MatchmakingState::Waiting) {
        return;
    }

    if (menu.Matchmaking.RecordId.empty()) {
        return;
    }

    pb.LeaveQueue(pb.LoggedUser.id, [](std::expected<std::string, PBError>) {
        // Fire and forget
    });

    menu.Matchmaking = MatchmakingContext{};
}

void UISystem::onJoinResult(const JoinResultEvent& aEvent)
{
    Registry* reg  = aEvent.Reg;
    auto&     menu = reg->ctx().get<MenuContext>();

    menu.Matchmaking.State    = MatchmakingState::Waiting;
    menu.Matchmaking.RecordId = aEvent.ID;

    WATO_INFO(*reg, "joined matchmaking queue, record id: {}", aEvent.ID);
}

void UISystem::onMatchmakingError(const MatchmakingErrorEvent& aEvent)
{
    Registry& registry = *aEvent.Reg;
    auto&     menu     = registry.ctx().get<MenuContext>();

    menu.Matchmaking.State = MatchmakingState::Failed;
    menu.Error             = aEvent.Error;
    menu.Message.clear();

    WATO_ERR(registry, "matchmaking error: {}", aEvent.Error);
}
