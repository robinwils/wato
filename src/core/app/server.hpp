#pragma once

#include "core/app/app.hpp"

class Server : public Application
{
   public:
    explicit Server(int aWidth, int aHeight) : Application(aWidth, aHeight) {}
    virtual ~Server() = default;

    Server(const Server &)            = delete;
    Server(Server &&)                 = delete;
    Server &operator=(const Server &) = delete;
    Server &operator=(Server &&)      = delete;

    void Init();
    int  Run();
};
