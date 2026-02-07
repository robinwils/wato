#pragma once

#include <entt/signal/dispatcher.hpp>
#include <string>

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
    MenuState        State;
    entt::dispatcher Dispatcher;

    // Login state
    ::LoginState LoginState = LoginState::Idle;
    std::string  LoginError{};

    MatchmakingContext Matchmaking;
};
