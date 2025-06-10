#pragma once

#include <argh.h>

struct Options {
    explicit Options(char** aArgv) : mParser({"--loglevel", "--renderer"}) { mParser.parse(aArgv); }

    [[nodiscard]] bool        Multiplayer() const noexcept { return mParser["multi"]; }
    [[nodiscard]] std::string LogLevel() const noexcept
    {
        return mParser("loglevel", "info").str();
    }

    [[nodiscard]] std::string Renderer() const noexcept
    {
        return mParser("renderer", "vulkan").str();
    }

   private:
    argh::parser mParser;
};
