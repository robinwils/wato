#pragma once

#include <fmt/format.h>

#include <atomic>
#include <functional>
#include <glaze/glaze.hpp>
#include <glaze/json/write.hpp>
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
        mAsyncResponses.emplace_back(Client.PostAsync<RegisterResult, PocketBaseErrorResponse>(
            "/api/collections/users/records",
            std::move(aCallback),
            cpr::Parameters{{"fields", "id,accountName"}},
            cpr::Payload{
                {"accountName", aAccount},
                {"password", aPassword},
                {"passwordConfirm", aPassword}}));
    }

    void RefreshToken(AsyncCallback<RefreshResult> aCallback)
    {
        mAsyncResponses.emplace_back(Client.PostAsync<RefreshResult, PocketBaseErrorResponse>(
            "/api/collections/users/auth-refresh",
            std::move(aCallback),
            AuthHeader(),
            cpr::Parameters{{"fields", "token"}}));
    }

    void JoinQueue(
        const std::string&               aID,
        int                              aTeamSize,
        int                              aTeamCount,
        AsyncCallback<MatchmakingRecord> aCallback)
    {
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
        mAsyncResponses.emplace_back(Client.DeleteAsync<std::string, PocketBaseErrorResponse>(
            "/api/collections/matchmaking_queue/records/" + aRecordId,
            std::move(aCallback),
            AuthHeader()));
    }

    /**
     * @brief Subscribe to a record via SSE
     *
     * @param aRecordId The record ID to subscribe to
     * @param aCallback Callback invoked when record is updated (from SSE thread, must be
     * thread-safe)
     */
    template <typename T>
    void Subscribe(
        const std::string&      aSubscription,
        AsyncCallback<PBSSE<T>> aCallback,
        const std::string&      aFields = "")
    {
        Unsubscribe(aSubscription);

        mSubscriptions.try_emplace(aSubscription);
        auto& sub = mSubscriptions.at(aSubscription);

        sub.SSEResponse = Client.ConnectSSE(
            "/api/realtime",
            [this, &sub, aSubscription, aFields, cb = std::move(aCallback)](
                const SSEEvent& aEvent) {
                mLogger->debug("got SSEEvent {}: {}", aEvent.Event, aEvent.Data);
                if (sub.StopSource.stop_requested()) {
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
                    cb(std::move(*record), "");
                } else {
                    mLogger->error("cannot decode sse record: {}", aEvent.Data);
                    cb(std::nullopt, "cannot decode server event");
                }
                return true;
            },
            AuthHeader());
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
        std::erase_if(mAsyncResponses, [](const AsyncRespCB& aRes) {
            return aRes.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready;
        });
        std::erase_if(mSubscriptions, [this](const auto& aPair) {
            auto& [name, state] = aPair;
            if (state.SSEResponse.wait_for(std::chrono::milliseconds(0))
                == std::future_status::ready) {
                mLogger->warn("SSE connection closed for subscription '{}'", name);
                return true;
            }
            return false;
        });
    }

    cpr::Header AuthHeader() const { return cpr::Header{{"Authorization", Token}}; }

    HTTPClient  Client;
    std::string Token;

   private:
    struct SSEState {
        std::stop_source   StopSource;
        cpr::AsyncResponse SSEResponse{};
    };
    void sendSubscription(
        const std::string& aClientId,
        const std::string& aSubscription,
        const std::string& aFields = "")
    {
        std::string topic = aSubscription;
        if (!aFields.empty()) {
            glz::generic opts;
            opts["query"] = glz::generic{{"fields", aFields}};
            topic        += "?options=" + glz::write_json(opts).value_or("{}");
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

    std::vector<AsyncRespCB> mAsyncResponses;

    // SSE state
    std::unordered_map<std::string, SSEState> mSubscriptions;
};
