#pragma once

#include <cpr/cpr.h>

#include <functional>
#include <string>

#include "core/sys/log.hpp"

struct SSEEvent {
    std::string Id;
    std::string Event;
    std::string Data;
};

using SSECallback = std::function<bool(const SSEEvent&)>;
using RawCallback = std::function<void(cpr::Response)>;

class HTTPClient
{
   public:
    HTTPClient(std::string const& aURL, const Logger& aLogger) : mURL(aURL), mLogger(aLogger) {}

    const std::string& URL() const { return mURL; }

    template <typename... Args>
    cpr::Response Get(const std::string& aEndpoint, Args&&... aArgs)
    {
        return cpr::Get(cpr::Url(mURL + aEndpoint), std::forward<Args>(aArgs)...);
    }

    template <typename... Args>
    cpr::Response Post(const std::string& aEndpoint, Args&&... aArgs)
    {
        return cpr::Post(cpr::Url(mURL + aEndpoint), std::forward<Args>(aArgs)...);
    }

    template <typename... Args>
    cpr::Response Patch(const std::string& aEndpoint, Args&&... aArgs)
    {
        return cpr::Patch(cpr::Url(mURL + aEndpoint), std::forward<Args>(aArgs)...);
    }

    template <typename... Args>
    auto PostAsync(const std::string& aEndpoint, RawCallback aCallback, Args&&... aArgs)
    {
        return cpr::PostCallback(
            [callback = std::move(aCallback)](const cpr::Response& aResp) { callback(aResp); },
            cpr::Url(mURL + aEndpoint),
            std::forward<Args>(aArgs)...);
    }

    template <typename... Args>
    auto GetAsync(const std::string& aEndpoint, RawCallback aCallback, Args&&... aArgs)
    {
        return cpr::GetCallback(
            [callback = std::move(aCallback)](const cpr::Response& aResp) { callback(aResp); },
            cpr::Url(mURL + aEndpoint),
            std::forward<Args>(aArgs)...);
    }

    template <typename... Args>
    auto PatchAsync(const std::string& aEndpoint, RawCallback aCallback, Args&&... aArgs)
    {
        return cpr::PatchCallback(
            [callback = std::move(aCallback)](const cpr::Response& aResp) { callback(aResp); },
            cpr::Url(mURL + aEndpoint),
            std::forward<Args>(aArgs)...);
    }

    template <typename... Args>
    auto DeleteAsync(const std::string& aEndpoint, RawCallback aCallback, Args&&... aArgs)
    {
        return cpr::DeleteCallback(
            [callback = std::move(aCallback)](const cpr::Response& aResp) { callback(aResp); },
            cpr::Url(mURL + aEndpoint),
            std::forward<Args>(aArgs)...);
    }

    template <typename... Args>
    auto ConnectSSE(const std::string& aEndpoint, SSECallback aCallback, Args&&... aArgs)
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
    std::string mURL;
    Logger      mLogger;
};
