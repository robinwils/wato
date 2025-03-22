#pragma once

#include <memory>

#include "core/physics.hpp"

class ServerEngine
{
   public:
    ServerEngine(std::unique_ptr<Physics> aPhy) : mPhysics(std::move(aPhy)) {}
    ServerEngine(ServerEngine &&)                 = delete;
    ServerEngine(const ServerEngine &)            = delete;
    ServerEngine &operator=(ServerEngine &&)      = delete;
    ServerEngine &operator=(const ServerEngine &) = delete;
    ~ServerEngine()                               = default;

    Physics &GetPhysics() { return *mPhysics; }

   private:
    std::unique_ptr<Physics> mPhysics;
};
