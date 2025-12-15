#pragma once

#include <argh.h>

struct Options {
    explicit Options() {}
    explicit Options(char** aArgv) : mParser({"--loglevel", "--renderer", "--server-addr"})
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
    std::string ServerAddr;

   private:
    argh::parser mParser;
};
