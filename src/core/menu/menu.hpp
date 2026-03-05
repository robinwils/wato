#pragma once

#include <entt/signal/dispatcher.hpp>
#include <string>

#include "core/menu/menu_backend.hpp"
#include "core/types.hpp"

enum class MenuState {
    Login,
    Register,
    Lobby,
    InGame,
    EndGame,
};

enum class LoginState {
    Idle,
    Pending,
    Success,
    Failed,
};

enum class RegisterState {
    Idle,
    Pending,
    Success,
    Failed,
};

enum class MatchmakingState {
    Idle,
    Joining,
    Waiting,
    Matched,
    Connecting,
    Failed,
};

struct MatchmakingContext {
    MatchmakingState State = MatchmakingState::Idle;
    std::string      RecordId;
    std::string      ServerAddr;
    GameInstanceID   MatchedGameId{0};
};

struct MenuContext {
    MenuContext(std::unique_ptr<MenuBackend>&& aBackend)
        : State(MenuState::Login), LoginState(LoginState::Idle), Backend(std::move(aBackend))
    {
    }

    void ClearMsgs()
    {
        Error.clear();
        Message.clear();
    }

    MenuState        State;
    entt::dispatcher Dispatcher{};

    ::LoginState    LoginState    = LoginState::Idle;
    ::RegisterState RegisterState = RegisterState::Idle;

    MatchmakingContext Matchmaking;

    std::string Error{};
    std::string Message{};

    std::unique_ptr<MenuBackend> Backend;
};
