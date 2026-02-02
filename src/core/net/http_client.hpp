#pragma once

#include <cpr/cpr.h>

#include <functional>
#include <optional>
#include <string>

template <typename T>
using AsyncCallback =
    std::function<void(const std::optional<T>& aResult, const std::string& aError)>;

class HTTPClient
{
   public:
    HTTPClient(std::string const& aURL) : mURL(aURL) {}

    template <typename T, typename... Args>
    void PostAsync(const std::string& aEndpoint, AsyncCallback<T> aCallback, Args&&... aArgs)
    {
        cpr::PostCallback(
            [callback = std::move(aCallback), this](const cpr::Response& aResp) {
                decodeResp(aResp, callback);
            },
            cpr::Url(mURL + aEndpoint),
            std::forward<Args>(aArgs)...);
    }

    template <typename T, typename... Args>
    void GetAsync(const std::string& aEndpoint, AsyncCallback<T> aCallback, Args&&... aArgs)
    {
        cpr::GetCallback(
            [callback = std::move(aCallback), this](const cpr::Response& aResp) {
                decodeResp(aResp, callback);
            },
            cpr::Url(mURL + aEndpoint),
            std::forward<Args>(aArgs)...);
    }

    template <typename T, typename... Args>
    void DeleteAsync(const std::string& aEndpoint, AsyncCallback<T> aCallback, Args&&... aArgs)
    {
        cpr::DeleteCallback(
            [callback = std::move(aCallback), this](const cpr::Response& aResp) {
                decodeResp(aResp, callback);
            },
            cpr::Url(mURL + aEndpoint),
            std::forward<Args>(aArgs)...);
    }

   private:
    template <typename T>
    void decodeResp(const cpr::Response& aResp, const AsyncCallback<T>& aCallback);

    std::string mURL;

    std::atomic_bool mRunning{false};
    std::atomic_bool mConnected{false};
};
