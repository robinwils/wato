#include "core/net/http_client.hpp"

#include <glaze/glaze.hpp>

template <typename T>
void HTTPClient::decodeResp(const cpr::Response& aResp, const AsyncCallback<T>& aCallback)
{
    if (aResp.status_code == 0) {
        aCallback(std::nullopt, aResp.error.message);
        return;
    }
    if (aResp.status_code >= 400) {
        aCallback(std::nullopt, "HTTP " + std::to_string(aResp.status_code));
        return;
    }

    auto res = glz::read_json<T>(aResp.text);
    if (!res) {
        aCallback(std::nullopt, "Failed to parse response");
        return;
    }

    callback(res.value(), "");
}

void HTTPClient::Subscribe(const std::string& aCollection, const std::string& aFilter)
{
    if (mRunning) {
        return;
    }

    mRunning = true;

    cpr::Header headers;
    if (!mToken.empty()) {
        headers["Authorization"] = mToken;
    }

    mAsyncResponse = cpr::GetAsync(
        cpr::Url{mBaseURL + "/api/realtime"},
        headers,
        cpr::ServerSentEventCallback{
            [this, collection = aCollection](cpr::ServerSentEvent&& aEvent, intptr_t) -> bool {
                return handleSSEEvent(std::move(aEvent), collection);
            }});
}

void HTTPClient::Unsubscribe()
{
    mRunning   = false;
    mConnected = false;
}

bool HTTPClient::handleSSEEvent(cpr::ServerSentEvent&& aEvent, const std::string& aCollection)
{
    if (!mRunning) {
        return false;
    }

    if (aEvent.event == "PB_CONNECT") {
        auto result = glz::read_json<PBConnectEvent>(aEvent.data);
        if (result) {
            mLogger->info("SSE: Connected with clientId {}", result->clientId);
            sendSubscription(result->clientId, aCollection, "");
            mConnected = true;
        } else {
            mLogger->error("SSE: Failed to parse PB_CONNECT event");
        }
        return true;
    }

    auto* evt = new SSEEvent{
        .Id    = aEvent.id,
        .Event = aEvent.event,
        .Data  = aEvent.data,
    };
    mEventChannel.Send(evt);

    return true;
}

void HTTPClient::sendSubscription(
    const std::string& aClientId,
    const std::string& aCollection,
    const std::string& aFilter)
{
    std::string subscription = aCollection;
    if (!aFilter.empty()) {
        subscription += "?filter=" + aFilter;
    }

    cpr::Header headers{{"Content-Type", "application/json"}};
    if (!mToken.empty()) {
        headers["Authorization"] = mToken;
    }

    // PocketBase subscription payload
    glz::json_t payload;
    payload["clientId"]      = aClientId;
    payload["subscriptions"] = std::vector<std::string>{subscription};

    auto response = cpr::Post(
        cpr::Url{mBaseURL + "/api/realtime"},
        headers,
        cpr::Body{glz::write_json(payload).value_or("{}")});

    if (response.status_code >= 400) {
        mLogger->error("SSE: Subscription failed with status {}", response.status_code);
    } else {
        mLogger->info("SSE: Subscribed to {}", subscription);
    }
}
