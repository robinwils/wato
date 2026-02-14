#pragma once

#include <string>

#include "registry/registry.hpp"

struct LoginEvent {
    Registry*   Reg;
    std::string Account;
    std::string Password;
};

struct LoginResultEvent {
    Registry*   Reg;
    std::string ID{};
    std::string Avatar{};
    std::string Email{};
    std::string AccountName{};
    std::string Token{};
    std::string Error{};
};

struct RegisterEvent {
    Registry*   Reg;
    std::string AccountName;
    std::string Password;
};

struct RegisterResultEvent {
    Registry*   Reg;
    std::string ID{};
    std::string Avatar{};
    std::string AccountName{};
    std::string Error{};
};

struct JoinMatchmakingEvent {
    Registry*    Reg;
    entt::entity Player;
};

struct LeaveMatchmakingEvent {
    Registry*    Reg;
    entt::entity Player;
};

struct JoinResultEvent {
    Registry*   Reg;
    std::string ID;
    std::string AccountName;
    std::string Status;
    std::string ServerAddr;
    std::string Created;
    std::string Updated;
};

struct MatchmakingErrorEvent {
    Registry*   Reg;
    std::string Error;
};
