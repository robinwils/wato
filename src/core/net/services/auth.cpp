#include "core/net/services/auth.hpp"

#include <cpr/cprtypes.h>

#include <glaze/glaze.hpp>

struct RefreshResponse {
    std::string token{};
};

std::optional<AuthService::LoginResult> AuthService::Login(
    std::string const& aAccount,
    std::string const& aPassword)
{
    std::string url = mURL + "/api/collections/users/auth-with-password";

    cpr::Response r = cpr::Post(
        cpr::Url{url},
        cpr::Parameters{{"fields", "record.avatar,record.email,record.name,token"}},
        cpr::Payload{{"identity", aAccount}, {"password", aPassword}});

    if (!CheckResponse(r, "login")) return std::nullopt;

    auto res = glz::read_json<AuthService::LoginResult>(r.text);
    if (!res) {
        mLogger->error("cannot decode login response: {}", glz::format_error(res, r.text));
        return std::nullopt;
    }
    mToken = res->token;
    return res.value();
}

bool AuthService::RefreshToken()
{
    std::string url = mURL + "/api/collections/users/auth-refresh";

    cpr::Response r = cpr::Post(cpr::Url{url}, AuthHeader(), cpr::Parameters{{"fields", "token"}});

    if (!CheckResponse(r, "refresh auth")) return false;

    auto res = glz::read_json<RefreshResponse>(r.text);
    if (!res) {
        mLogger->error("cannot decode auth refresh response: {}", glz::format_error(res, r.text));
        return false;
    }
    mToken = res->token;
    return true;
}
