#pragma once

#include <glaze/glaze.hpp>
#include <glaze/util/key_transformers.hpp>

#include "components/player.hpp"
#include "core/net/services/backend.hpp"

class AuthService : public BackendService
{
   public:
    struct Record {
        std::string avatar{};
        std::string email{};
        std::string name{};
    };
    struct LoginResult {
        Record      record{};
        std::string token{};
    };
    using BackendService::BackendService;

    std::optional<LoginResult> Login(const std::string& aAccount, const std::string& aPassword);
    LoginResult
    Register(const std::string& aUsername, const std::string& aTag, const std::string& aPassword);
    bool RefreshToken();

    cpr::Header AuthHeader() const { return cpr::Header{{"Authorization", mToken}}; }

   private:
    std::string mToken;
};
