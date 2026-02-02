#include "core/net/pocketbase.hpp"

void PocketBaseClient::Login(
    std::string const&         aAccount,
    std::string const&         aPassword,
    AsyncCallback<LoginResult> aCallback)
{
    mClient.PostAsync<LoginResult>(
        "/api/collections/users/auth-with-password",
        std::move(aCallback),
        cpr::Parameters{{"fields", "record.avatar,record.email,record.accountName,token"}},
        cpr::Payload{{"identity", aAccount}, {"password", aPassword}});
}

void PocketBaseClient::RefreshToken(AsyncCallback<RefreshResult> aCallback)
{
    mClient.PostAsync<RefreshResult>(
        "/api/collections/users/auth-refresh",
        std::move(aCallback),
        AuthHeader(),
        cpr::Parameters{{"fields", "token"}});
}
