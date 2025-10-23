#define ENET_IMPLEMENTATION
#define ENET_FEATURE_ADDRESS_MAPPING
#include <signal.h>

#include <memory>
#include <taskflow/core/taskflow.hpp>

#include "core/app/game_client.hpp"
#include "core/app/game_server.hpp"
#include "core/options.hpp"
#include "core/sys/backtrace.hpp"
#include "core/sys/log.hpp"

int main(int, char** argv)
{
#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
    signal(SIGSEGV, signalHandler);
#endif

    Options opts(argv);

    tf::Taskflow                flow;
    tf::Executor                exec;
    tf::Future<void>            future;
    std::unique_ptr<GameServer> server;

    if (opts.ServerAddr == "") {
        opts.ServerAddr = "any:7777";
        server          = std::make_unique<GameServer>(opts);

        flow.emplace([&]() {
            server->Init();
            server->Run(exec);
        });
        exec.run(flow);
    }

    GameClient game(1920, 1080, opts);
    game.Init();

    game.Run(exec);
    if (server) {
        server->Stop();
    }
    exec.wait_for_all();
    return 0;
}
