#pragma once

#include <cpr/cpr.h>

#include <functional>
#include <optional>
#include <string>

#include "core/sys/log.hpp"

template <typename T>
using AsyncCallback =
    std::function<void(const std::optional<T>& aResult, const std::string& aError)>;

struct SSEEvent {
    std::string Id;
    std::string Event;
    std::string Data;
};

using SSECallback = std::function<bool(const SSEEvent&)>;

class HTTPClient
{
   public:
    HTTPClient(std::string const& aURL, const Logger& aLogger) : mURL(aURL), mLogger(aLogger) {}

    const std::string& URL() const { return mURL; }

    template <typename T, typename... Args>
    void Post(const std::string& aEndpoint, Args&&... aArgs)
    {
        cpr::Post(cpr::Url(mURL + aEndpoint), std::forward<Args>(aArgs)...);
    }

    template <typename T, typename ErrT, typename... Args>
    void PostAsync(const std::string& aEndpoint, AsyncCallback<T> aCallback, Args&&... aArgs)
    {
        cpr::PostCallback(
            [callback = std::move(aCallback), this](const cpr::Response& aResp) {
                decodeResp<T, ErrT>(aResp, callback);
            },
            cpr::Url(mURL + aEndpoint),
            std::forward<Args>(aArgs)...);
    }

    template <typename T, typename ErrT, typename... Args>
    void GetAsync(const std::string& aEndpoint, AsyncCallback<T> aCallback, Args&&... aArgs)
    {
        cpr::GetCallback(
            [callback = std::move(aCallback), this](const cpr::Response& aResp) {
                decodeResp<T, ErrT>(aResp, callback);
            },
            cpr::Url(mURL + aEndpoint),
            std::forward<Args>(aArgs)...);
    }

    template <typename T, typename ErrT, typename... Args>
    void DeleteAsync(const std::string& aEndpoint, AsyncCallback<T> aCallback, Args&&... aArgs)
    {
        cpr::DeleteCallback(
            [callback = std::move(aCallback), this](const cpr::Response& aResp) {
                decodeResp<T, ErrT>(aResp, callback);
            },
            cpr::Url(mURL + aEndpoint),
            std::forward<Args>(aArgs)...);
    }

    // SSE - returns AsyncResponse for caller to store
    template <typename... Args>
    cpr::AsyncResponse
    ConnectSSE(const std::string& aEndpoint, SSECallback aCallback, Args&&... aArgs)
    {
        return cpr::GetAsync(
            cpr::Url{mURL + aEndpoint},
            cpr::ServerSentEventCallback{
                [cb = std::move(aCallback)](cpr::ServerSentEvent&& ev, intptr_t) -> bool {
                    return cb(SSEEvent{
                        .Id    = ev.id.value_or(""),
                        .Event = std::move(ev.event),
                        .Data  = std::move(ev.data),
                    });
                }},
            std::forward<Args>(aArgs)...);
    }

   private:
    template <typename T, typename ErrT>
    void decodeResp(const cpr::Response& aResp, const AsyncCallback<T>& aCallback);

    std::string mURL;
    Logger      mLogger;
};

#include "core/net/http_client.hxx"
