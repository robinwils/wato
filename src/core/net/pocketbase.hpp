#pragma once

#include <optional>
#include <string>

#include "core/net/services/auth.hpp"
#include "registry/registry.hpp"

enum class LoginState {
    Idle,
    Pending,
    Success,
    Failed,
};

struct LoginContext {
    LoginState                              State = LoginState::Idle;
    std::string                             Error;
    std::optional<AuthService::LoginResult> Result;
};

struct LoginResultEvent {
    Registry*                               Reg;
    std::optional<AuthService::LoginResult> Result;
    std::string                             Error;
};

/**
 * @brief Unified client for all PocketBase/backend operations
 *
 * Contains services and their associated state contexts.
 * Stored in registry context. State updated on main thread only.
 */
struct PocketBaseClient {
    AuthService  Auth;
    LoginContext LoginCtx;

    PocketBaseClient(const std::string& aURL, const Logger& aLogger) : Auth(aURL, aLogger) {}

    void SetToken(const std::string& aToken) { Auth.SetToken(aToken); }
};
