#pragma once

#include <argh.h>

struct Options {
    Options(char** aArgv) : mParser({"--loglevel"}) { mParser.parse(aArgv); }
    Options(const Options&)            = default;
    Options(Options&&)                 = default;
    Options& operator=(const Options&) = default;
    Options& operator=(Options&&)      = default;
    ~Options()                         = default;

    [[nodiscard]] bool        Multiplayer() const noexcept { return mParser["multi"]; }
    [[nodiscard]] std::string LogLevel() const noexcept
    {
        return mParser("loglevel", "info").str();
    }

   private:
    argh::parser mParser;
};
