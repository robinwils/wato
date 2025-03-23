#include "core/app/server.hpp"

#include <bx/bx.h>

#include "core/physics.hpp"
#include "systems/system.hpp"

void Server::Init()
{
    auto &physics = mRegistry.ctx().emplace<Physics>();

    physics.Init();

    mSystems.push_back(PhysicsSystem::MakeDelegate(mPhysicsSystem));
}

int Server::Run()
{
    using clock   = std::chrono::high_resolution_clock;
    auto prevTime = clock::now();

    return 0;
}
