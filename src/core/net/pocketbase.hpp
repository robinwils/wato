#pragma once

#include <atomic>
#include <functional>
#include <glaze/glaze.hpp>
#include <glaze/json/write.hpp>
#include <optional>
#include <string>

#include "core/net/http_client.hpp"
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
    std::string    id{};
    std::string    accountName{};
    std::string    status{};
    GameInstanceID gameId{0};
    std::string    serverAddr{};
    std::string    created{};
    std::string    updated{};
};

struct ServerMatchRequest {
    std::string RecordId;
    std::string UserId;
};

template <typename T>
struct PBSSE {
    std::string action{};
    T           record{};
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
    PocketBaseClient(const std::string& aURL, const Logger& aLogger, const std::string& aToken = "")
        : Client(aURL, aLogger), Token(aToken), mLogger(aLogger)
    {
    }

    void Login(
        const std::string&         aAccount,
        const std::string&         aPassword,
        AsyncCallback<LoginResult> aCallback)
    {
        Client.PostAsync<LoginResult, PocketBaseErrorResponse>(
            "/api/collections/users/auth-with-password",
            std::move(aCallback),
            cpr::Parameters{
                {"fields", "record.id,record.avatar,record.email,record.accountName,token"}},
            cpr::Payload{{"identity", aAccount}, {"password", aPassword}});
    }

    void Register(
        const std::string&            aAccount,
        const std::string&            aPassword,
        AsyncCallback<RegisterResult> aCallback)
    {
        Client.PostAsync<RegisterResult, PocketBaseErrorResponse>(
            "/api/collections/users/records",
            std::move(aCallback),
            cpr::Parameters{{"fields", "id,accountName"}},
            cpr::Payload{
                {"accountName", aAccount},
                {"password", aPassword},
                {"passwordConfirm", aPassword}});
    }

    void RefreshToken(AsyncCallback<RefreshResult> aCallback)
    {
        Client.PostAsync<RefreshResult, PocketBaseErrorResponse>(
            "/api/collections/users/auth-refresh",
            std::move(aCallback),
            AuthHeader(),
            cpr::Parameters{{"fields", "token"}});
    }

    void JoinQueue(
        const std::string&               aID,
        int                              aTeamSize,
        int                              aTeamCount,
        AsyncCallback<MatchmakingRecord> aCallback)
    {
        Client.PostAsync<MatchmakingRecord, PocketBaseErrorResponse>(
            "/api/collections/matchmaking_queue/records",
            std::move(aCallback),
            cpr::Header{{"Authorization", Token}, {"Content-Type", "application/json"}},
            cpr::Parameters{{"fields", "id,accountName,ceated,status"}},
            cpr::Body{glz::write_json(glz::generic{
                                          {"accountName", aID},
                                          {"status", "waiting"},
                                          {"teamSize", aTeamSize},
                                          {"teamCount", aTeamCount}})
                          .value_or("{}")});
    }

    void LeaveQueue(const std::string& aRecordId, AsyncCallback<std::string> aCallback)
    {
        Client.DeleteAsync<std::string, PocketBaseErrorResponse>(
            "/api/collections/matchmaking_queue/records/" + aRecordId,
            std::move(aCallback),
            AuthHeader());
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
        Unsubscribe();
        mSSERunning = true;

        mSSEResponse = Client.ConnectSSE(
            "/api/realtime",
            [this, aSubscription, aFields, cb = std::move(aCallback)](const SSEEvent& aEvent) {
                mLogger->debug("got SSEEvent {}: {}", aEvent.Event, aEvent.Data);
                if (!mSSERunning) {
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
                    mLogger->info("got SSE");
                    cb(*record, "");
                } else {
                    mLogger->error("cannot decode sse record: {}", aEvent.Data);
                    cb(std::nullopt, "cannot decode server event");
                }
                return true;
            },
            AuthHeader());
    }

    void Unsubscribe()
    {
        mSSERunning = false;
        mLogger->debug("unsubscribed");
    }

    bool IsSubscribed() const { return mSSERunning.load(); }

    cpr::Header AuthHeader() const { return cpr::Header{{"Authorization", Token}}; }

    HTTPClient  Client;
    std::string Token;

   private:
    void sendSubscription(
        const std::string& aClientId,
        const std::string& aSubscription,
        const std::string& aFields = "")
    {
        std::string topic = aSubscription;
        if (!aFields.empty()) {
            glz::generic opts;
            opts["query"]    = glz::generic{{"fields", aFields}};
            opts["headers"]  = glz::generic{};
            topic           += "?options=" + glz::write_json(opts).value_or("{}");
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

    // SSE state
    cpr::AsyncResponse mSSEResponse;
    std::atomic_bool   mSSERunning{false};
};
