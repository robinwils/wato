#pragma once

#include <entt/signal/dispatcher.hpp>

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

struct MenuContext {
    MenuState        State;
    entt::dispatcher Dispatcher;

    ::LoginState LoginState = LoginState::Idle;
    std::string  LoginError{};
};
