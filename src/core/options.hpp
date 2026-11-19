#pragma once

#include <argh.h>

struct Options {
    explicit Options()                 = default;
    Options(const Options&)            = default;
    Options(Options&&)                 = default;
    Options& operator=(const Options&) = default;
    Options& operator=(Options&&)      = default;

    explicit Options(char** aArgv)
        : mParser(
              {"--loglevel",
               "--renderer",
               "--server-addr",
               "--backend-addr",
               "--user",
               "--password"})
    {
        mParser.parse(aArgv);
        ServerAddr = mParser("server-addr", "").str();
    }

    [[nodiscard]] std::string LogLevel() const noexcept
    {
        return mParser("loglevel", "info").str();
    }

    [[nodiscard]] std::string Renderer() const noexcept
    {
        return mParser("renderer", "vulkan").str();
    }

    [[nodiscard]] std::string BackendAddr() const noexcept
    {
        return mParser("backend-addr", "http://localhost:8090").str();
    }

    [[nodiscard]] std::string User() const noexcept
    {
        return mParser("user", "").str();
    }

    [[nodiscard]] std::string Password() const noexcept { return mParser("password", "").str(); }

    std::string ServerAddr;

   private:
    argh::parser mParser;
};
