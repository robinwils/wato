
#include <sodium/core.h>

#include "core/app/game_server.hpp"
#include "core/sys/signal.hpp"

int main(int, char** argv)
{
    installSignalHandlers();

    if (sodium_init() == -1) {
        spdlog::error("cannot initialize lib sodium");
        return 1;
    }

    tf::Executor exec;
    Options      opts(argv);

    if (opts.ServerAddr == "") {
        opts.ServerAddr = "127.0.0.1:7777";
    }

    char*       emailEnv = getenv("ADMIN_EMAIL");
    char*       passEnv  = getenv("ADMIN_PASSWORD");
    std::string email    = emailEnv ? emailEnv : "";
    std::string password = passEnv ? passEnv : "";

    GameServer s(opts, email, password);
    s.Init();
    return s.Run(exec);
}
