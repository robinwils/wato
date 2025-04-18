#include "core/app/game_server.hpp"

#include <bx/bx.h>

#include <thread>

#include "core/physics.hpp"
#include "input/action.hpp"
#include "systems/system.hpp"

void GameServer::Init()
{
    mServer.Init();
    auto& physics = mRegistry.ctx().get<Physics>();

    physics.Init(mRegistry);

    mSystems.push_back(PhysicsSystem::MakeDelegate(mPhysicsSystem));
    mSystems.push_back(CreepSystem::MakeDelegate(mCreepSystem));
}

int GameServer::Run()
{
    using clock   = std::chrono::high_resolution_clock;
    auto prevTime = clock::now();

    mRunning = true;

    std::jthread netPollThread{[&]() {
        while (mRunning) {
            mServer.Poll();
        }
    }};

    ActionBuffer rb;

    while (mRunning) {
        auto                         t  = clock::now();
        std::chrono::duration<float> dt = (t - prevTime);
        prevTime                        = t;

        mServer.ConsumeEvents(&mRegistry);

        for (const auto& system : mSystems) {
            system(mRegistry, dt.count());
        }
    }

    return 0;
}
