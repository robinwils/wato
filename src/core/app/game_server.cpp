#include "core/app/game_server.hpp"

#include <bx/bx.h>
#include <fmt/base.h>

#include <thread>

#include "components/game.hpp"
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
        delete ev;
    }
}

int GameServer::Run()
{
    auto prevTime = clock_type::now();
    mRunning      = true;

    std::jthread netPollThread{[&]() {
        while (mRunning) {
            mServer.Poll();
        }
    }};

    while (mRunning) {
        auto                         t  = clock_type::now();
        std::chrono::duration<float> dt = (t - prevTime);
        prevTime                        = t;

        ConsumeNetworkEvents();
        advanceSimulations(dt.count());
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
    registry.ctx().emplace<ActionBuffer>();
    registry.ctx().emplace<GameInstance>(gameID, 0.0f, 0u);

    physics.Init(registry);
    return gameID;
}

void GameServer::advanceSimulations(const float aDeltaTime)
{
    static constexpr float kTimeStep = 1.0f / 60.0f;

    // Update each game instance independently
    for (auto& [gameId, registry] : mGameInstances) {
        auto& actions  = registry.ctx().get<ActionBuffer&>();
        auto& instance = registry.ctx().get<GameInstance&>();

        instance.Accumulator += aDeltaTime;

        // While there is enough accumulated time to take
        // one or several physics steps
        while (instance.Accumulator >= kTimeStep) {
            // Decrease the accumulated time
            instance.Accumulator -= kTimeStep;

            for (const auto& system : mSystemsFT) {
                system(registry, kTimeStep);
            }
            actions.Push();
            actions.Latest().Tick = ++instance.Tick;
        }
    }
}
