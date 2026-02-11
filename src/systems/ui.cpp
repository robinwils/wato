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
    aDispatcher.sink<RegisterEvent>().connect<&UISystem::onRegister>(*this);
    aDispatcher.sink<RegisterResultEvent>().connect<&UISystem::onRegisterResult>(*this);
    aDispatcher.sink<JoinMatchmakingEvent>().connect<&UISystem::onJoinMatchmaking>(*this);
    aDispatcher.sink<LeaveMatchmakingEvent>().connect<&UISystem::onLeaveMatchmaking>(*this);
    aDispatcher.sink<JoinResultEvent>().connect<&UISystem::onJoinResult>(*this);
    aDispatcher.sink<MatchFoundEvent>().connect<&UISystem::onMatchFound>(*this);
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
        [reg](const std::optional<LoginResult>& aResult, const std::string& aError) {
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
                dispatcher.enqueue<LoginResultEvent>(LoginResultEvent{.Reg = reg, .Error = aError});
            }
        });
}

void UISystem::onLoginResult(const LoginResultEvent& aEvent)
{
    Registry& registry = *aEvent.Reg;

    auto& pb   = registry.ctx().get<PocketBaseClient>();
    auto& menu = registry.ctx().get<MenuContext>();

    if (aEvent.Error.empty()) {
        entt::entity player = registry.create();

        registry.emplace<RecordID>(player, aEvent.ID);
        registry.emplace<Email>(player, aEvent.Email);
        registry.emplace<AccountName>(player, aEvent.AccountName);
        registry.ctx().emplace_as<entt::entity>("player"_hs, player);

        menu.LoginState = LoginState::Success;
        pb.Token        = aEvent.Token;
        menu.ClearMsgs();

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
        [reg](const std::optional<RegisterResult>& aResult, const std::string& aError) {
            auto& dispatcher = reg->ctx().get<MenuContext>().Dispatcher;
            if (aResult) {
                dispatcher.enqueue<RegisterResultEvent>(RegisterResultEvent{
                    .Reg         = reg,
                    .ID          = aResult->id,
                    .AccountName = aResult->accountName,
                    .Error       = ""});
            } else {
                dispatcher.enqueue<RegisterResultEvent>(
                    RegisterResultEvent{.Reg = reg, .Error = aError});
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
    auto&     id   = reg->get<RecordID>(aEvent.Player);

    if (menu.Matchmaking.State != MatchmakingState::Idle) {
        return;
    }

    menu.Matchmaking.State = MatchmakingState::Joining;
    menu.ClearMsgs();

    pb.JoinQueue(
        id.Value,
        1,
        2,
        [reg](const std::optional<MatchmakingRecord>& aResult, const std::string& aError) {
            auto& dispatcher = reg->ctx().get<MenuContext>().Dispatcher;
            if (aResult) {
                dispatcher.enqueue<JoinResultEvent>(JoinResultEvent{
                    .Reg         = reg,
                    .ID          = aResult->id,
                    .AccountName = aResult->accountName,
                    .Status      = aResult->status,
                    .GameId      = aResult->gameId,
                    .ServerAddr  = aResult->serverAddr,
                    .Created     = aResult->created,
                    .Updated     = aResult->updated,
                });
            } else {
                dispatcher.enqueue<MatchmakingErrorEvent>(
                    MatchmakingErrorEvent{.Reg = reg, .Error = aError});
            }
        });
}

void UISystem::onLeaveMatchmaking(const LeaveMatchmakingEvent& aEvent)
{
    Registry* reg = aEvent.Reg;

    auto& pb   = reg->ctx().get<PocketBaseClient>();
    auto& menu = reg->ctx().get<MenuContext>();
    auto& id   = reg->get<RecordID>(aEvent.Player);

    if (menu.Matchmaking.State == MatchmakingState::Idle) {
        return;
    }

    pb.Unsubscribe();

    if (!menu.Matchmaking.RecordId.empty()) {
        pb.LeaveQueue(id.Value, [](const std::optional<std::string>&, const std::string&) {
            // Fire and forget
        });
    }

    menu.Matchmaking = MatchmakingContext{};
}

void UISystem::onJoinResult(const JoinResultEvent& aEvent)
{
    Registry* reg  = aEvent.Reg;
    auto&     pb   = reg->ctx().get<PocketBaseClient>();
    auto&     menu = reg->ctx().get<MenuContext>();

    menu.Matchmaking.State    = MatchmakingState::Waiting;
    menu.Matchmaking.RecordId = aEvent.ID;

    pb.Subscribe<MatchmakingRecord>(
        "matchmaking_queue/" + aEvent.ID,
        [reg](const std::optional<PBSSE<MatchmakingRecord>>& aRecord, const std::string& aError) {
            if (aRecord && aRecord->record.status == "matched" && aRecord->record.gameId != 0) {
                auto& dispatcher = reg->ctx().get<MenuContext>().Dispatcher;
                dispatcher.enqueue<MatchFoundEvent>(MatchFoundEvent{
                    .Reg        = reg,
                    .GameId     = aRecord->record.gameId,
                    .ServerAddr = aRecord->record.serverAddr,
                });
            }
        },
        "action,record.id,record.accountName,record.status,record.gameID,record.serverAddr,record."
        "creatd,record.updated");

    WATO_INFO(*reg, "joined matchmaking queue, record id: {}", aEvent.ID);
}

void UISystem::onMatchFound(const MatchFoundEvent& aEvent)
{
    Registry& registry = *aEvent.Reg;
    auto&     pb       = registry.ctx().get<PocketBaseClient>();
    auto&     menu     = registry.ctx().get<MenuContext>();

    menu.Matchmaking.State         = MatchmakingState::Matched;
    menu.Matchmaking.MatchedGameId = aEvent.GameId;
    menu.Matchmaking.ServerAddr    = aEvent.ServerAddr;

    pb.Unsubscribe();

    WATO_INFO(registry, "match found! game_id: {}, server: {}", aEvent.GameId, aEvent.ServerAddr);
}

void UISystem::onMatchmakingError(const MatchmakingErrorEvent& aEvent)
{
    Registry& registry = *aEvent.Reg;
    auto&     pb       = registry.ctx().get<PocketBaseClient>();
    auto&     menu     = registry.ctx().get<MenuContext>();

    menu.Matchmaking.State = MatchmakingState::Failed;
    menu.Error             = aEvent.Error;
    menu.Message.clear();

    pb.Unsubscribe();

    WATO_ERR(registry, "matchmaking error: {}", aEvent.Error);
}
