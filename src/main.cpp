#define ENET_IMPLEMENTATION
#include <signal.h>

#include "core/app/game_client.hpp"
#include "core/sys/backtrace.hpp"

int main(int, char** argv)
{
    signal(SIGSEGV, signalHandler);

    GameClient game(1920, 1080, argv);
    game.Init();

    return game.Run();
}
