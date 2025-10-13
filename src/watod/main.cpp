
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

    tf::Executor exec;
    Options      opts(argv);

    if (opts.ServerAddr == "") {
        opts.ServerAddr = "127.0.0.1:7777";
    }

    InitLogger(opts.LogLevel());

    GameServer s(opts);
    s.Init();
    return s.Run(exec);
}
