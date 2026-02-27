#pragma once

#include <fmt/format.h>

#include <atomic>
#include <chrono>
#include <expected>
#include <functional>
#include <glaze/glaze.hpp>
#include <glaze/json/write.hpp>
#include <mutex>
#include <string>
#include <vector>

#include "core/net/http_client.hpp"
#include "core/queue/channel.hpp"
#include "core/sys/log.hpp"
#include "core/types.hpp"

struct LoginRecord {
    std::string id;
    std::string avatar{};
    std::string email{};
    std::string accountName{};
};

struct LoginResult {
    LoginRecord record{};
    std::string token{};
};

struct RegisterResult {
    std::string id{};
    std::string accountName{};
};

struct RefreshResult {
    std::string token{};
};

struct PBConnectEvent {
    std::string clientId;
};

struct MatchmakingRecord {
    std::string id{};
    std::string accountName{};
    std::string status{};
    std::string game{};
    std::string serverAddr{};
    std::string created{};
    std::string updated{};
};

template <>
struct fmt::formatter<MatchmakingRecord> : fmt::formatter<std::string> {
    auto format(MatchmakingRecord const& aObj, format_context& aCtx) const -> decltype(aCtx.out())
    {
        return fmt::format_to(
            aCtx.out(),
            "MatchmakingRecord: id = {}, accountName = {}, status = {}, game = {}, serverAddr = "
            "{}, created = {}, updated = {}",
            aObj.id,
            aObj.accountName,
            aObj.status,
            aObj.game,
            aObj.serverAddr,
            aObj.created,
            aObj.updated);
    }
};

struct ServerMatchRequest {
    std::string RecordId;
    std::string UserId;
};

struct GameRecord {
    std::string              id{};
    std::vector<std::string> players;
    std::string              created{};
};

template <>
struct fmt::formatter<GameRecord> : fmt::formatter<std::string> {
    auto format(GameRecord const& aObj, format_context& aCtx) const -> decltype(aCtx.out())
    {
        return fmt::format_to(
            aCtx.out(),
            "GameRecord: id = {}, players = {}",
            aObj.id,
            aObj.players);
    }
};

template <typename T>
struct PBList {
    std::vector<T> items;
    int            page{};
    int            perPage{};
    int            totalItems{};
    int            totalPages{};
};

using GameRecordList = PBList<GameRecord>;

struct GameServerRecord {
    static constexpr const char* kFields = "id,ip,port,publicKey,hasAESNI,created,updated";

    std::string id{};
    std::string ip{};
    int         port{};
    std::string publicKey{};
    bool        hasAESNI{};
    std::string created{};
    std::string updated{};
};

using GameServerRecordList = PBList<GameServerRecord>;

template <typename T>
struct PBSSE {
    std::string action{};
    T           record{};
};

template <typename T>
struct fmt::formatter<PBSSE<T>> : fmt::formatter<std::string> {
    auto format(PBSSE<T> const& aObj, format_context& aCtx) const -> decltype(aCtx.out())
    {
        return fmt::format_to(aCtx.out(), "SSE {} Event: {}", aObj.action, aObj.record);
    }
};

struct PocketBaseErrorResponse {
    glz::generic data{};
    std::string  message{};
    long         status{};
};

struct PBError {
    long        StatusCode{};
    std::string Message;
};

template <typename T>
using PBCallback = std::function<void(std::expected<T, PBError>)>;

/**
 * @brief Unified client for all PocketBase/backend operations
 *
 * Contains services and their associated state contexts.
 * Stored in registry context. State updated on main thread only.
 */
class PocketBaseClient
{
   public:
    using AsyncRespCB = cpr::AsyncWrapper<void, true>;

    PocketBaseClient(const PocketBaseClient&)            = delete;
    PocketBaseClient(PocketBaseClient&&)                 = delete;
    PocketBaseClient& operator=(const PocketBaseClient&) = delete;
    PocketBaseClient& operator=(PocketBaseClient&&)      = delete;
    PocketBaseClient(
        const std::string& aURL,
        const Logger&      aLogger,
        const std::string& aToken = "");

    void Login(
        const std::string&      aAccount,
        const std::string&      aPassword,
        PBCallback<LoginResult> aCallback);

    void Register(
        const std::string&         aAccount,
        const std::string&         aPassword,
        PBCallback<RegisterResult> aCallback);

    void RefreshToken(PBCallback<LoginResult> aCallback, const std::string& aToken = "");

    void JoinQueue(
        const std::string&            aID,
        int                           aTeamSize,
        int                           aTeamCount,
        PBCallback<MatchmakingRecord> aCallback);

    void LeaveQueue(const std::string& aRecordId, PBCallback<std::string> aCallback);

    template <typename T>
    void Subscribe(
        const std::string&    aSubscription,
        Channel<PBSSE<T>>&    aChan,
        const std::string&    aFields      = "",
        std::function<void()> aOnReconnect = nullptr)
    {
        Unsubscribe(aSubscription);

        mSubscriptions.try_emplace(aSubscription);
        auto& sub = mSubscriptions.at(aSubscription);

        sub.OnReconnect = std::move(aOnReconnect);
        sub.Factory     = [this, aSubscription, aFields, &aChan](std::stop_source aStop) {
            return startSSE<T>(aSubscription, aFields, aChan, aStop);
        };
        sub.SSEResponse = sub.Factory(sub.StopSource);
    }

    void GetGamesSince(const std::string& aTimestamp, PBCallback<GameRecordList> aCallback);

    void UpdateGame(
        const std::string&     aRecord,
        const std::string&     aStatus,
        PBCallback<GameRecord> aCallback);

    std::expected<GameServerRecord, PBError> RegisterGameServerSync(
        const std::string& aIp,
        int                aPort,
        const std::string& aPubKey,
        bool               aHasAESNI);

    void Unsubscribe(const std::string& aSubscription);
    bool IsSubscribed(const std::string& aSubscription) const;

    void Update();

    cpr::Header AuthHeader(const std::string& aToken = "") const;

    HTTPClient  Client;
    std::string Token;
    LoginRecord LoggedUser;

   private:
    template <typename T>
    std::expected<T, PBError> decodePBResponse(const cpr::Response& aResp)
    {
        if (aResp.status_code == 0) {
            return std::unexpected(PBError{.StatusCode = 0, .Message = aResp.error.message});
        }
        if (aResp.status_code >= 400) {
            auto errorJSON = glz::read_json<PocketBaseErrorResponse>(aResp.text);
            auto codeStr   = std::to_string(aResp.status_code);

            if (!errorJSON) {
                mLogger->error("failed to decode error json {}", aResp.text);
                return std::unexpected(PBError{
                    .StatusCode = aResp.status_code,
                    .Message    = fmt::format("Failed to parse {} error response", codeStr)});
            }
            return std::unexpected(PBError{
                .StatusCode = aResp.status_code,
                .Message    = fmt::format("HTTP {}: {}", codeStr, errorJSON->message)});
        }

        auto res = glz::read_json<T>(aResp.text);
        if (!res) {
            mLogger->error("failed to decode json {}", aResp.text);
            return std::unexpected(
                PBError{.StatusCode = aResp.status_code, .Message = "Failed to parse response"});
        }

        return std::move(res.value());
    }

    template <typename T, typename... Args>
    void postAsync(const std::string& aEndpoint, PBCallback<T> aCallback, Args&&... aArgs)
    {
        auto future = Client.PostAsync(
            aEndpoint,
            [cb = std::move(aCallback), this](cpr::Response aResp) {
                cb(decodePBResponse<T>(aResp));
            },
            std::forward<Args>(aArgs)...);
        std::lock_guard lock(mAsyncMutex);
        mAsyncResponses.emplace_back(std::move(future));
    }

    template <typename T, typename... Args>
    void getAsync(const std::string& aEndpoint, PBCallback<T> aCallback, Args&&... aArgs)
    {
        auto future = Client.GetAsync(
            aEndpoint,
            [cb = std::move(aCallback), this](cpr::Response aResp) {
                cb(decodePBResponse<T>(aResp));
            },
            std::forward<Args>(aArgs)...);
        std::lock_guard lock(mAsyncMutex);
        mAsyncResponses.emplace_back(std::move(future));
    }

    template <typename T, typename... Args>
    void patchAsync(const std::string& aEndpoint, PBCallback<T> aCallback, Args&&... aArgs)
    {
        auto future = Client.PatchAsync(
            aEndpoint,
            [cb = std::move(aCallback), this](cpr::Response aResp) {
                cb(decodePBResponse<T>(aResp));
            },
            std::forward<Args>(aArgs)...);
        std::lock_guard lock(mAsyncMutex);
        mAsyncResponses.emplace_back(std::move(future));
    }

    template <typename T, typename... Args>
    void deleteAsync(const std::string& aEndpoint, PBCallback<T> aCallback, Args&&... aArgs)
    {
        auto future = Client.DeleteAsync(
            aEndpoint,
            [cb = std::move(aCallback), this](cpr::Response aResp) {
                cb(decodePBResponse<T>(aResp));
            },
            std::forward<Args>(aArgs)...);
        std::lock_guard lock(mAsyncMutex);
        mAsyncResponses.emplace_back(std::move(future));
    }

    struct SSEState {
        std::stop_source   StopSource;
        cpr::AsyncResponse SSEResponse{};

        bool                                                ShouldReconnect{false};
        std::chrono::steady_clock::time_point               NextReconnect{};
        std::chrono::milliseconds                           BackoffMs{1000};
        std::function<void()>                               OnReconnect;
        std::function<cpr::AsyncResponse(std::stop_source)> Factory;
    };

    template <typename T>
    bool handleSSEEvent(
        const SSEEvent&    aEvent,
        Channel<PBSSE<T>>& aChan,
        std::stop_source   aStop,
        const std::string& aSubscription,
        const std::string& aFields)
    {
        mLogger->debug("got SSEEvent {}: {}", aEvent.Event, aEvent.Data);
        if (aStop.stop_requested()) {
            mLogger->info("Stop requested for subscription {}", aSubscription);
            return false;
        }

        if (aEvent.Event == "PB_CONNECT") {
            auto connect = glz::read_json<PBConnectEvent>(aEvent.Data);
            if (connect) {
                mLogger->info("SSE: Connected with clientId {}", connect->clientId);
                sendSubscription(connect->clientId, aSubscription, aFields);
            } else {
                mLogger->error("SSE: Failed to parse PB_CONNECT event");
            }
            return true;
        }

        auto record = glz::read_json<PBSSE<T>>(aEvent.Data);
        if (record) {
            mLogger->info("got SSE: {}", *record);
            aChan.Send(new PBSSE<T>(std::move(*record)));
        } else {
            mLogger->error("cannot decode sse record: {}", aEvent.Data);
        }
        return true;
    }

    template <typename T>
    cpr::AsyncResponse startSSE(
        const std::string& aSubscription,
        const std::string& aFields,
        Channel<PBSSE<T>>& aChan,
        std::stop_source   aStop)
    {
        return Client.ConnectSSE(
            "/api/realtime",
            [this, stopSource = aStop, aSubscription, aFields, &aChan](
                const SSEEvent& aEvent) mutable {
                return handleSSEEvent<T>(aEvent, aChan, stopSource, aSubscription, aFields);
            },
            AuthHeader());
    }

    void sendSubscription(
        const std::string& aClientId,
        const std::string& aSubscription,
        const std::string& aFields = "");

    Logger mLogger;

    std::mutex               mAsyncMutex;
    std::vector<AsyncRespCB> mAsyncResponses;

    // SSE state
    std::unordered_map<std::string, SSEState> mSubscriptions;
};
