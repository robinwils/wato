#include "core/app/server.hpp"

#include <bx/bx.h>

#include <memory>

#include "core/server_engine.hpp"
#include "systems/system.hpp"

void Server::Init()
{
    auto &engine = mRegistry.ctx().emplace<ServerEngine>(std::make_unique<Physics>());

    engine.GetPhysics().Init(&mRegistry);

    mSystems.push_back(PhysicsSystem::MakeDelegate(mPhysicsSystem));

#if WATO_DEBUG
    mSystems.push_back(PhysicsDebugSystem::MakeDelegate(mPhysicsDbgSystem));
#endif
}

int Server::Run()
{
    using clock   = std::chrono::high_resolution_clock;
    auto prevTime = clock::now();

    return 0;
}
