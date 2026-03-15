#include <sodium/core.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <taskflow/core/taskflow.hpp>

#include "core/app/game_client.hpp"
#include "core/app/game_server.hpp"
#include "core/options.hpp"
#include "core/sys/signal.hpp"
#include "core/sys/log.hpp"

int main(int, char** argv)
{
    installSignalHandlers();

    Options opts(argv);

    tf::Taskflow                flow;
    tf::Executor                exec;
    tf::Future<void>            future;
    std::unique_ptr<GameServer> server;

    char*       emailEnv = getenv("ADMIN_EMAIL");
    char*       passEnv  = getenv("ADMIN_PASSWORD");
    std::string email    = emailEnv ? emailEnv : "";
    std::string password = passEnv ? passEnv : "";

    if (sodium_init() == -1) {
        spdlog::error("cannot initialize lib sodium");
        return 1;
    }

    if (opts.ServerAddr == "") {
        opts.ServerAddr = "any:7777";
        server          = std::make_unique<GameServer>(opts, email, password);

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
