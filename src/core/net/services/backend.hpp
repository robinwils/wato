#pragma once

#include <cpr/cpr.h>
#include <spdlog/spdlog.h>

#include <functional>
#include <glaze/glaze.hpp>
#include <optional>
#include <string>

#include "core/net/services/http_client.hpp"
#include "core/sys/log.hpp"

template <typename T>
using AsyncCallback =
    std::function<void(const std::optional<T>& aResult, const std::string& aError)>;

class BackendService
{
   public:
    BackendService(std::string const& aURL, Logger const& aLogger) : mClient(aURL), mLogger(aLogger)
    {
    }

   protected:
    HTTPClient mClient;
    Logger     mLogger;
};
