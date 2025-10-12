#include <signal.h>

#define ENET_IMPLEMENTATION
#include "core/app/game_server.hpp"
#include "core/sys/backtrace.hpp"

int main(int, char** argv)
{
#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
    signal(SIGSEGV, signalHandler);
#endif

    Options      opts(argv);

    InitLogger(opts.LogLevel());

    GameServer s(opts);
    s.Init();
    return s.Run();
}
