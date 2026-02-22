#pragma once

#include <fmt/format.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <glaze/glaze.hpp>
#include <glaze/json/write.hpp>
#include <mutex>
#include <optional>
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

struct GameRecordList {
    std::vector<GameRecord> items;
};

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
    PocketBaseClient(const std::string& aURL, const Logger& aLogger, const std::string& aToken = "")
        : Client(aURL, aLogger), Token(aToken), mLogger(aLogger)
    {
    }

    void Login(
        const std::string&         aAccount,
        const std::string&         aPassword,
        AsyncCallback<LoginResult> aCallback)
    {
        std::lock_guard lock(mAsyncMutex);
        mAsyncResponses.emplace_back(Client.PostAsync<LoginResult, PocketBaseErrorResponse>(
            "/api/collections/users/auth-with-password",
            std::move(aCallback),
            cpr::Parameters{
                {"fields", "record.id,record.avatar,record.email,record.accountName,token"}},
            cpr::Payload{{"identity", aAccount}, {"password", aPassword}}));
    }

    void Register(
        const std::string&            aAccount,
        const std::string&            aPassword,
        AsyncCallback<RegisterResult> aCallback)
    {
        std::lock_guard lock(mAsyncMutex);
        mAsyncResponses.emplace_back(Client.PostAsync<RegisterResult, PocketBaseErrorResponse>(
            "/api/collections/users/records",
            std::move(aCallback),
            cpr::Parameters{{"fields", "id,accountName"}},
            cpr::Payload{
                {"accountName", aAccount},
                {"password", aPassword},
                {"passwordConfirm", aPassword}}));
    }

    void RefreshToken(AsyncCallback<LoginResult> aCallback, const std::string& aToken = "")
    {
        std::lock_guard lock(mAsyncMutex);
        mAsyncResponses.emplace_back(Client.PostAsync<LoginResult, PocketBaseErrorResponse>(
            "/api/collections/users/auth-refresh",
            std::move(aCallback),
            AuthHeader(aToken),
            cpr::Parameters{{"fields", "record.id,record.accountName,token"}}));
    }

    void JoinQueue(
        const std::string&               aID,
        int                              aTeamSize,
        int                              aTeamCount,
        AsyncCallback<MatchmakingRecord> aCallback)
    {
        std::lock_guard lock(mAsyncMutex);
        mAsyncResponses.emplace_back(Client.PostAsync<MatchmakingRecord, PocketBaseErrorResponse>(
            "/api/collections/matchmaking_queue/records",
            std::move(aCallback),
            cpr::Header{{"Authorization", Token}, {"Content-Type", "application/json"}},
            cpr::Parameters{{"fields", "id,accountName,ceated,status"}},
            cpr::Body{glz::write_json(glz::generic{
                                          {"accountName", aID},
                                          {"status", "waiting"},
                                          {"teamSize", aTeamSize},
                                          {"teamCount", aTeamCount}})
                          .value_or("{}")}));
    }

    void LeaveQueue(const std::string& aRecordId, AsyncCallback<std::string> aCallback)
    {
        std::lock_guard lock(mAsyncMutex);
        mAsyncResponses.emplace_back(Client.DeleteAsync<std::string, PocketBaseErrorResponse>(
            "/api/collections/matchmaking_queue/records/" + aRecordId,
            std::move(aCallback),
            AuthHeader()));
    }

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

    void GetGamesSince(const std::string& aTimestamp, AsyncCallback<GameRecordList> aCallback)
    {
        std::lock_guard lock(mAsyncMutex);
        mAsyncResponses.emplace_back(Client.GetAsync<GameRecordList, PocketBaseErrorResponse>(
            "/api/collections/game/records",
            std::move(aCallback),
            AuthHeader(),
            cpr::Parameters{
                {"filter", fmt::format("created>\"{}\"", aTimestamp)},
                {"fields", "id,players,status,created"}}));
    }

    void Unsubscribe(const std::string& aSubscription)
    {
        if (auto it = mSubscriptions.find(aSubscription); it != mSubscriptions.end()) {
            it->second.StopSource.request_stop();
            mSubscriptions.erase(it);
            mLogger->debug("unsubscribed");
        }
    }

    bool IsSubscribed(const std::string& aSubscription) const
    {
        return mSubscriptions.contains(aSubscription);
    }

    void Update()
    {
        {
            std::lock_guard lock(mAsyncMutex);
            std::erase_if(mAsyncResponses, [](const AsyncRespCB& aRes) {
                return aRes.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready;
            });
        }

        auto now = std::chrono::steady_clock::now();

        std::vector<std::function<void()>> toReconnect;

        for (auto& [name, state] : mSubscriptions) {
            if (!state.ShouldReconnect && state.SSEResponse.valid()
                && state.SSEResponse.wait_for(std::chrono::milliseconds(0))
                       == std::future_status::ready) {
                auto resp = state.SSEResponse.get();

                if (state.StopSource.stop_requested()) {
                    continue;
                }

                state.ShouldReconnect = true;
                if (resp.status_code >= 400) {
                    mLogger->warn(
                        "SSE '{}' closed with HTTP {}, backoff {}ms",
                        name,
                        resp.status_code,
                        state.BackoffMs.count());
                    state.NextReconnect = now + state.BackoffMs;
                    state.BackoffMs =
                        std::min(state.BackoffMs * 2, std::chrono::milliseconds(30000));
                } else {
                    mLogger->warn(
                        "SSE '{}' closed (status {}), reconnecting in 1s",
                        name,
                        resp.status_code);
                    state.NextReconnect = now + std::chrono::milliseconds(1000);
                    state.BackoffMs     = std::chrono::milliseconds(1000);
                }
            }

            if (state.ShouldReconnect && now >= state.NextReconnect) {
                mLogger->info("reconnecting SSE '{}'", name);
                state.StopSource      = std::stop_source{};
                state.SSEResponse     = state.Factory(state.StopSource);
                state.ShouldReconnect = false;
                if (state.OnReconnect) toReconnect.push_back(state.OnReconnect);
            }
        }

        for (auto& cb : toReconnect) cb();
    }

    cpr::Header AuthHeader(const std::string& aToken = "") const
    {
        return cpr::Header{{"Authorization", aToken.empty() ? Token : aToken}};
    }

    HTTPClient  Client;
    std::string Token;
    LoginRecord LoggedUser;

   private:
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
        const std::string& aFields = "")
    {
        std::string topic = aSubscription;
        if (!aFields.empty()) {
            glz::generic opts;
            opts["query"]  = glz::generic{{"fields", aFields}};
            topic         += "?options=" + glz::write_json(opts).value_or("{}");
        }

        glz::generic payload;
        payload["clientId"]      = aClientId;
        payload["subscriptions"] = std::vector<std::string>{topic};

        Client.Post<std::string>(
            "/api/realtime",
            cpr::Header{{"Content-Type", "application/json"}, {"Authorization", Token}},
            cpr::Body{glz::write_json(payload).value_or("{}")});

        mLogger->info("SSE: Subscribed to {}", topic);
    }

    Logger mLogger;

    std::mutex               mAsyncMutex;
    std::vector<AsyncRespCB> mAsyncResponses;

    // SSE state
    std::unordered_map<std::string, SSEState> mSubscriptions;
};
