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
        std::string accountName{};
    };
    struct LoginResult {
        Record      record{};
        std::string token{};
    };
    struct RefreshResult {
        std::string token{};
    };

    using BackendService::BackendService;

    void Login(const std::string& aAccount, const std::string& aPassword, AsyncCallback<LoginResult> aCallback);
    void RefreshToken(AsyncCallback<RefreshResult> aCallback);

    cpr::Header AuthHeader() const { return cpr::Header{{"Authorization", mToken}}; }
    void        SetToken(const std::string& aToken) { mToken = aToken; }

   private:
    std::string mToken;
};
