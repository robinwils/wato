#pragma once

#include <argh.h>

struct Options {
    Options(char** aArgv) : mParser(aArgv) {}
    Options(const Options&)            = default;
    Options(Options&&)                 = default;
    Options& operator=(const Options&) = default;
    Options& operator=(Options&&)      = default;
    ~Options()                         = default;

    [[nodiscard]] bool Multiplayer() const noexcept { return mParser["multi"]; }

   private:
    argh::parser mParser;
};
