#pragma once

#include <argh.h>

struct Options {
    explicit Options(char** aArgv) : mParser({"--loglevel"}) { mParser.parse(aArgv); }

    [[nodiscard]] bool        Multiplayer() const noexcept { return mParser["multi"]; }
    [[nodiscard]] std::string LogLevel() const noexcept
    {
        return mParser("loglevel", "info").str();
    }

   private:
    argh::parser mParser;
};
