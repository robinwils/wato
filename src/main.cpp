#define ENET_IMPLEMENTATION
#include <signal.h>

#include "core/app/game_client.hpp"
#include "core/sys/backtrace.hpp"

int main(int, char** argv)
{
#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
    signal(SIGSEGV, signalHandler);
#endif

    GameClient game(1920, 1080, argv);
    game.Init();

    return game.Run();
}
