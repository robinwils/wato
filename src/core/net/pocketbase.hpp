#pragma once

#include <optional>
#include <string>

#include "core/net/http_client.hpp"
#include "core/sys/log.hpp"
#include "registry/registry.hpp"

struct Record {
    std::string avatar{};
    std::string email{};
    std::string accountName{};
};

struct LoginResult {
    Record      record{};
    std::string token{};
};

struct RefreshResult {
    std::string token{};
};

enum class LoginState {
    Idle,
    Pending,
    Success,
    Failed,
};

struct LoginContext {
    LoginState                 State = LoginState::Idle;
    std::string                Error;
    std::optional<LoginResult> Result;
};

/**
 * @brief Unified client for all PocketBase/backend operations
 *
 * Contains services and their associated state contexts.
 * Stored in registry context. State updated on main thread only.
 */
class PocketBaseClient
{
   public:
    PocketBaseClient(const std::string& aURL, const Logger& aLogger)
        : Client(aURL), mLogger(aLogger)
    {
    }

    void Login(
        const std::string&         aAccount,
        const std::string&         aPassword,
        AsyncCallback<LoginResult> aCallback)
    {
        Client.PostAsync<LoginResult>(
            "/api/collections/users/auth-with-password",
            std::move(aCallback),
            cpr::Parameters{{"fields", "record.avatar,record.email,record.accountName,token"}},
            cpr::Payload{{"identity", aAccount}, {"password", aPassword}});
    }

    void RefreshToken(AsyncCallback<RefreshResult> aCallback)
    {
        Client.PostAsync<RefreshResult>(
            "/api/collections/users/auth-refresh",
            std::move(aCallback),
            AuthHeader(),
            cpr::Parameters{{"fields", "token"}});
    }

    cpr::Header AuthHeader() const { return cpr::Header{{"Authorization", Token}}; }

    HTTPClient  Client;
    std::string Token;

   private:
    Logger mLogger;
};
