#define ENET_IMPLEMENTATION
#include <signal.h>

#include "core/app/game_client.hpp"
#include "core/sys/backtrace.hpp"
#include "core/sys/log.hpp"

int main(int, char** argv)
{
#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
    signal(SIGSEGV, signalHandler);
#endif

    Options opts(argv);

    InitLogger(opts.LogLevel());
    GameClient game(1920, 1080, opts);
    game.Init();

    return game.Run();
}
