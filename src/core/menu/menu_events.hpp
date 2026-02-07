#pragma once

#include <string>

#include "core/types.hpp"
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

struct JoinMatchmakingEvent {
    Registry*    Reg;
    entt::entity Player;
};

struct LeaveMatchmakingEvent {
    Registry*    Reg;
    entt::entity Player;
};

struct JoinResultEvent {
    Registry*      Reg;
    std::string    ID;
    std::string    AccountName;
    std::string    Status;
    GameInstanceID GameId;
    std::string    ServerAddr;
    std::string    Created;
    std::string    Updated;
};

struct MatchFoundEvent {
    Registry*      Reg;
    GameInstanceID GameId;
    std::string    ServerAddr{};
};

struct MatchmakingErrorEvent {
    Registry*   Reg;
    std::string Error;
};
