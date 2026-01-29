#pragma once

#include <cpr/cpr.h>
#include <spdlog/spdlog.h>

#include <string>

#include "core/sys/log.hpp"

class BackendService
{
   public:
    BackendService(std::string const& aURL, Logger const& aLogger) : mURL(aURL), mLogger(aLogger) {}

   protected:
    bool CheckResponse(cpr::Response const& aResp, std::string const& aMsg)
    {
        if (aResp.status_code == 0) {
            mLogger->error("cannot {}: {}", aMsg, aResp.error.message);
            return false;
        } else if (aResp.status_code >= 400) {
            mLogger->error(
                "cannot {}: {}: {}",
                aMsg,
                aResp.status_code,
                std::to_string(aResp.error.code));
            return false;
        }
        return true;
    }

    std::string mURL;
    Logger      mLogger;
};
