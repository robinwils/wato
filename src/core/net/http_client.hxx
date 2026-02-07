#pragma once

#include <glaze/glaze.hpp>

template <typename T, typename ErrT>
void HTTPClient::decodeResp(const cpr::Response& aResp, const AsyncCallback<T>& aCallback)
{
    if (aResp.status_code == 0) {
        aCallback(std::nullopt, aResp.error.message);
        return;
    }
    if (aResp.status_code >= 400) {
        auto errorJSON = glz::read_json<ErrT>(aResp.text);
        auto codeStr   = std::to_string(aResp.status_code);

        if (!errorJSON) {
            mLogger->error("failed to decode error json {}", aResp.text);
            aCallback(std::nullopt, fmt::format("Failed to parse {} error response", codeStr));
            return;
        }
        aCallback(std::nullopt, fmt::format("HTTP {}: {}", codeStr, errorJSON->message));
        return;
    }

    auto res = glz::read_json<T>(aResp.text);
    if (!res) {
        mLogger->error("failed to decode json {}", aResp.text);
        aCallback(std::nullopt, "Failed to parse response");
        return;
    }

    aCallback(res.value(), "");
}
