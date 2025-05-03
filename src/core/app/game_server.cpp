#include "core/app/game_server.hpp"

#include <bx/bx.h>
#include <fmt/base.h>

#include <thread>

#include "core/net/net.hpp"
#include "core/physics.hpp"
#include "core/types.hpp"
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
    while (const NetworkEvent<NetworkRequestPayload>* ev = mServer.Queue().pop()) {
        std::visit(
            EventVisitor{
                [&](const PlayerActions& aActions) {
                    if (!mGameInstances.contains(aActions.GameID)) {
                        fmt::println("got event for non existing game {}", aActions.GameID);
                        return;
                    }
                    Registry& registry = mGameInstances[aActions.GameID];
                    auto&     actions  = registry.ctx().get<ActionBuffer&>().Latest().Actions;
                    fmt::println("got {} actions", aActions.Actions.size());
                    actions.insert(actions.end(), aActions.Actions.begin(), aActions.Actions.end());
                },
                [&](const NewGameRequest& aNewGame) {
                    GameInstanceID gameID = createGameInstance(aNewGame);
                    mServer.EnqueueResponse(new NetworkEvent<NetworkResponsePayload>{
                        .Type    = PacketType::NewGame,
                        .Payload = NewGameResponse{.GameID = gameID}});
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

GameInstanceID GameServer::createGameInstance(const NewGameRequest& aNewGame)
{
    GameInstanceID gameID;
    do {
        gameID = GenerateGameInstanceID();
    } while (mGameInstances.contains(gameID));

    Registry& registry = mGameInstances[gameID];
    auto&     physics  = registry.ctx().emplace<Physics>();
    registry.ctx().emplace<GameInstanceID>(gameID);

    physics.Init(registry);
    return gameID;
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
