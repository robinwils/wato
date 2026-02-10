#pragma once

#include <entt/signal/dispatcher.hpp>
#include <string>

#include "core/menu/menu_backend.hpp"
#include "core/types.hpp"

enum class MenuState {
    MainMenu,
    InGame,
    EndGame,
};

enum class LoginState {
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
    std::string      Error;
    std::string      ServerAddr;
    GameInstanceID   MatchedGameId{0};
};

struct MenuContext {
    MenuContext(std::unique_ptr<MenuBackend>&& aBackend)
        : State(MenuState::MainMenu), LoginState(LoginState::Idle), Backend(std::move(aBackend))
    {
    }

    MenuState        State;
    entt::dispatcher Dispatcher{};

    // Login state
    ::LoginState LoginState = LoginState::Idle;
    std::string  LoginError{};

    MatchmakingContext Matchmaking;

    std::unique_ptr<MenuBackend> Backend;
};
