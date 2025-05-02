#include "core/app/game_server.hpp"

#include <bx/bx.h>

#include <thread>

#include "core/physics.hpp"
#include "input/action.hpp"
#include "registry/registry.hpp"
#include "systems/system.hpp"

void GameServer::Init()
{
    mServer.Init();
    mSystemsFT.push_back(PhysicsSystem::MakeDelegate(mPhysicsSystem));
    mSystemsFT.push_back(CreepSystem::MakeDelegate(mCreepSystem));
    mSystemsFT.push_back(DeterministicActionSystem::MakeDelegate(mFTActionSystem));
}

void GameServer::ConsumeNetworkEvents()
{
    while (const NetworkEvent* ev = mServer.Queue().pop()) {
        auto& actions = mRegistry.ctx().get<ActionBuffer&>().Latest().Actions;
        std::visit(
            EventVisitor{
                [&](const PlayerActions& aActions) {
                    fmt::println("got {} actions", aActions.Actions.size());
                    actions.insert(actions.end(), aActions.Actions.begin(), aActions.Actions.end());
                },
                [&](const NewGamePayload& aNewGame) {

                },
            },
            ev->Payload);
    }
}

int GameServer::Run()
{
    auto prevTime = clock_type::now();

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

        ConsumeNetworkEvents();
    }

    return 0;
}

void GameServer::createGameInstance(const std::string& aGameName)
{
    if (!mGameInstances.contains(aGameName)) {
        Registry& registry = mGameInstances[aGameName];
        auto&     physics  = registry.ctx().emplace<Physics>();

        physics.Init(registry);
    }
}
void GameServer::advanceSimulation(Registry& aRegistry)
{
    uint32_t tick        = 0;
    float    accumulator = 0.0f;
    auto&    actions     = aRegistry.ctx().get<ActionBuffer&>();
    // While there is enough accumulated time to take
    // one or several physics steps
    while (accumulator >= kTimeStep) {
        // Decrease the accumulated time
        accumulator -= kTimeStep;

        for (const auto& system : mSystemsFT) {
            system(aRegistry, kTimeStep);
        }
        actions.Push();
        actions.Latest().Tick = ++tick;
    }
}
