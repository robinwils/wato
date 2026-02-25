
#include <sodium/core.h>
#define ENET_IMPLEMENTATION
#define ENET_FEATURE_ADDRESS_MAPPING
#include <signal.h>

#include "core/app/game_server.hpp"
#include "core/sys/backtrace.hpp"

int main(int, char** argv)
{
#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
    signal(SIGSEGV, signalHandler);
#endif

    if (sodium_init() == -1) {
        spdlog::error("cannot initialize lib sodium");
        return 1;
    }

    tf::Executor exec;
    Options      opts(argv);

    if (opts.ServerAddr == "") {
        opts.ServerAddr = "127.0.0.1:7777";
    }

    char*       tokenEnv = getenv("PB_TOKEN");
    std::string token    = tokenEnv ? tokenEnv : "";

    GameServer s(opts, token);
    s.Init();
    return s.Run(exec);
}
