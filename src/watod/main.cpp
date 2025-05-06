#include <signal.h>

#define ENET_IMPLEMENTATION
#include "core/app/game_server.hpp"
#include "core/sys/backtrace.hpp"

int main(int, char** argv)
{
    signal(SIGSEGV, signalHandler);

    GameServer s(argv);
    s.Init();
    return s.Run();
}
