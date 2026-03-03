#include "core/net/pocketbase.hpp"

PocketBaseClient::PocketBaseClient(
    const std::string& aURL,
    const Logger&      aLogger,
    const std::string& aToken)
    : Client(aURL, aLogger), Token(aToken), mLogger(aLogger)
{
}

void PocketBaseClient::Login(
    const std::string&      aAccount,
    const std::string&      aPassword,
    PBCallback<LoginResult> aCallback)
{
    postAsync<LoginResult>(
        "/api/collections/users/auth-with-password",
        std::move(aCallback),
        cpr::Parameters{{"fields", LoginResult::kFields}},
        cpr::Payload{{"identity", aAccount}, {"password", aPassword}});
}

void PocketBaseClient::Register(
    const std::string&         aAccount,
    const std::string&         aPassword,
    PBCallback<RegisterResult> aCallback)
{
    postAsync<RegisterResult>(
        "/api/collections/users/records",
        std::move(aCallback),
        cpr::Parameters{{"fields", "id,accountName"}},
        cpr::Payload{
            {"accountName", aAccount},
            {"password", aPassword},
            {"passwordConfirm", aPassword}});
}

void PocketBaseClient::RefreshToken(PBCallback<LoginResult> aCallback, const std::string& aToken)
{
    postAsync<LoginResult>(
        "/api/collections/users/auth-refresh",
        std::move(aCallback),
        AuthHeader(aToken),
        cpr::Parameters{{"fields", "record.id,record.accountName,token"}});
}

void PocketBaseClient::JoinQueue(
    const std::string&            aID,
    int                           aTeamSize,
    int                           aTeamCount,
    PBCallback<MatchmakingRecord> aCallback)
{
    postAsync<MatchmakingRecord>(
        "/api/collections/matchmaking_queue/records",
        std::move(aCallback),
        cpr::Header{{"Authorization", Token}, {"Content-Type", "application/json"}},
        cpr::Parameters{{"fields", "id,accountName,created,status"}},
        cpr::Body{glz::write_json(glz::generic{
                                      {"accountName", aID},
                                      {"status", "waiting"},
                                      {"teamSize", aTeamSize},
                                      {"teamCount", aTeamCount}})
                      .value_or("{}")});
}

void PocketBaseClient::LeaveQueue(const std::string& aRecordId, PBCallback<std::string> aCallback)
{
    deleteAsync<std::string>(
        "/api/collections/matchmaking_queue/records/" + aRecordId,
        std::move(aCallback),
        AuthHeader());
}

void PocketBaseClient::GetGamesSince(
    const std::string&         aTimestamp,
    PBCallback<GameRecordList> aCallback)
{
    getAsync<GameRecordList>(
        "/api/collections/game/records",
        std::move(aCallback),
        AuthHeader(),
        cpr::Parameters{
            {"filter", fmt::format("created>\"{}\"", aTimestamp)},
            {"fields", "id,players,status,created"}});
}

void PocketBaseClient::UpdateGame(
    const std::string&     aRecord,
    const std::string&     aStatus,
    PBCallback<GameRecord> aCallback)
{
    patchAsync<GameRecord>(
        "/api/collections/game/records/" + aRecord,
        std::move(aCallback),
        AuthHeader(),
        cpr::Payload{{"status", aStatus}},
        cpr::Parameters{{"fields", "id,players,status,created"}});
}

std::expected<std::string, PBError> PocketBaseClient::LoginSuperuserSync(
    const std::string& aEmail,
    const std::string& aPassword)
{
    auto resp = Client.Post(
        "/api/collections/_superusers/auth-with-password",
        cpr::Parameters{{"fields", LoginResult::kFields}},
        cpr::Payload{{"identity", aEmail}, {"password", aPassword}});

    auto result = decodePBResponse<LoginResult>(resp);
    if (!result) {
        return std::unexpected(result.error());
    }

    Token = result->token;
    return Token;
}

std::expected<GameServerRecord, PBError> PocketBaseClient::GetGameServer(
    const std::string& aIp,
    int                aPort)
{
    auto r = decodePBResponse<GameServerRecordList>(Client.Get(
        "/api/collections/game_servers/records",
        AuthHeader(),
        cpr::Parameters{
            {"filter", fmt::format("(ip='{}' && port={})", aIp, aPort)},
            {"fields", GameServerRecord::kFields},
        }));

    if (!r) {
        return std::unexpected(r.error());
    }
    if (r->items.empty()) {
        return std::unexpected(PBError{.StatusCode = 404, .Message = "Game server not found"});
    }
    return std::move(r->items[0]);
}

std::expected<GameServerRecord, PBError> PocketBaseClient::RegisterGameServerSync(
    const std::string& aIp,
    int                aPort,
    const std::string& aPubKey,
    bool               aHasAESNI)
{
    auto fields = cpr::Parameter{"fields", GameServerRecord::kFields};

    auto r = decodePBResponse<GameServerRecordList>(Client.Get(
        "/api/collections/game_servers/records",
        AuthHeader(),
        cpr::Parameters{
            {"filter", fmt::format("(ip='{}' && port={})", aIp, aPort)},
            fields,
        }));

    glz::generic payload;
    payload["ip"]        = aIp;
    payload["port"]      = std::to_string(aPort);
    payload["publicKey"] = aPubKey;
    payload["hasAESNI"]  = aHasAESNI;
    auto json            = glz::write_json(payload).value_or("{}");
    auto body            = cpr::Body{glz::write_json(payload).value_or("{}")};

    cpr::Header headers     = AuthHeader();
    headers["Content-Type"] = "application/json";

    if (r && !r->items.empty()) {
        mLogger->debug("server already registered, patching.");
        return decodePBResponse<GameServerRecord>(Client.Patch(
            "/api/collections/game_servers/records/" + r->items[0].id,
            headers,
            body,
            cpr::Parameters{fields}));
    } else if (r->items.empty()) {
        mLogger->debug("server unregistered, adding.");
        return decodePBResponse<GameServerRecord>(Client.Post(
            "/api/collections/game_servers/records",
            headers,
            body,
            cpr::Parameters{fields}));
    } else {
        return std::unexpected{r.error()};
    }
}

void PocketBaseClient::Unsubscribe(const std::string& aSubscription)
{
    if (auto it = mSubscriptions.find(aSubscription); it != mSubscriptions.end()) {
        it->second.StopSource->store(true);
        mSubscriptions.erase(it);
        mLogger->debug("unsubscribed");
    }
}

bool PocketBaseClient::IsSubscribed(const std::string& aSubscription) const
{
    return mSubscriptions.contains(aSubscription);
}

void PocketBaseClient::Update()
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

            if (state.StopSource->load()) {
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
                state.BackoffMs = std::min(state.BackoffMs * 2, std::chrono::milliseconds(30000));
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
            state.StopSource      = std::make_shared<std::atomic<bool>>(false);
            state.SSEResponse     = state.Factory(state.StopSource);
            state.ShouldReconnect = false;
            if (state.OnReconnect) toReconnect.push_back(state.OnReconnect);
        }
    }

    for (auto& cb : toReconnect) cb();
}

cpr::Header PocketBaseClient::AuthHeader(const std::string& aToken) const
{
    return cpr::Header{{"Authorization", aToken.empty() ? Token : aToken}};
}

void PocketBaseClient::sendSubscription(
    const std::string& aClientId,
    const std::string& aSubscription,
    const std::string& aFields)
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

    Client.Post(
        "/api/realtime",
        cpr::Header{{"Content-Type", "application/json"}, {"Authorization", Token}},
        cpr::Body{glz::write_json(payload).value_or("{}")});

    mLogger->info("SSE: Subscribed to {}", topic);
}
