#include "core/app/game_server.hpp"

#include <bx/bx.h>

#include <thread>

#include "core/physics.hpp"
#include "systems/system.hpp"

void GameServer::Init()
{
    mServer.Init();
    auto& physics = mRegistry.ctx().emplace<Physics>();

    physics.Init();

    mSystems.push_back(PhysicsSystem::MakeDelegate(mPhysicsSystem));
}

int GameServer::Run()
{
    using clock   = std::chrono::high_resolution_clock;
    auto prevTime = clock::now();

    mRunning = true;

    std::jthread netPollThread{[&]() {
        while (mRunning) {
            mServer.Poll(mQueue);
        }
    }};

    return 0;
}
