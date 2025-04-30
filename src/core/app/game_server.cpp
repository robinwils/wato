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

    mSystemsFT.push_back(PhysicsSystem::MakeDelegate(mPhysicsSystem));
    mSystemsFT.push_back(CreepSystem::MakeDelegate(mCreepSystem));
    mSystemsFT.push_back(DeterministicActionSystem::MakeDelegate(mFTActionSystem));
}

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
    constexpr float timeStep = 1.0f / 60.0f;

    uint32_t tick     = 0;
    auto     prevTime = clock_type::now();
    auto&    actions  = mRegistry.ctx().get<ActionBuffer&>();

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

        // While there is enough accumulated time to take
        // one or several physics steps
        while (accumulator >= timeStep) {
            // Decrease the accumulated time
            accumulator -= timeStep;

            for (const auto& system : mSystemsFT) {
                system(mRegistry, timeStep);
            }
            actions.Push();
            actions.Latest().Tick = ++tick;
        }
    }

    return 0;
}
