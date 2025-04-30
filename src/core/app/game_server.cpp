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

void GameServer::ConsumeNetworkEvents()
{
    auto& actions = mRegistry.ctx().get<ActionBuffer&>().Latest().Actions;

    EventVisitor visitor{
        [&](const PlayerActions& aActions) {
            fmt::println("got {} actions", aActions.Actions.size());
            actions.insert(actions.end(), aActions.Actions.begin(), aActions.Actions.end());
        },
        [&](const NewGamePayload& aNewGame) {

        },
    };
    while (NetworkEvent* ev = mServer.Queue().pop()) {
        std::visit(visitor, ev->Payload);
    }
}

int GameServer::Run()
{
    auto     prevTime = clock_type::now();

    mRunning = true;

    std::jthread netPollThread{[&]() {
        while (mRunning) {
            mServer.Poll();
        }
    }};

    ActionBuffer rb;

    while (mRunning) {
        auto                         t  = clock_type::now();
        std::chrono::duration<float> dt = (t - prevTime);
        prevTime                        = t;
        float accumulator               = 0.0f;

        ConsumeNetworkEvents();

        for (const auto& system : mSystems) {
            system(mRegistry, dt.count());
        }
    }

    return 0;
}
