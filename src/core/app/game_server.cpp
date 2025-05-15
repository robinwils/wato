#include "core/app/game_server.hpp"

#include <bx/bx.h>
#include <spdlog/spdlog.h>

#include <thread>

#include "core/net/net.hpp"
#include "core/types.hpp"
#include "input/action.hpp"
#include "registry/registry.hpp"
#include "systems/system.hpp"

void GameServer::Init()
{
    Application::Init();
    mServer.Init();
    mSystemsFT.push_back(PhysicsSystem::MakeDelegate(mPhysicsSystem));
    mSystemsFT.push_back(CreepSystem::MakeDelegate(mCreepSystem));
    mSystemsFT.push_back(ServerActionSystem::MakeDelegate(mActionSystem));
}

void GameServer::ConsumeNetworkRequests()
{
    while (const NetworkEvent<NetworkRequestPayload>* ev = mServer.Queue().pop()) {
        std::visit(
            VariantVisitor{
                [&](const PlayerActions& aActions) {
                    if (!mGameInstances.contains(aActions.GameID)) {
                        spdlog::info("got event for non existing game {}", aActions.GameID);
                        return;
                    }
                    Registry& registry = mGameInstances[aActions.GameID];
                    auto&     actions  = registry.ctx().get<ActionBuffer&>().Latest().Actions;
                    spdlog::info("got {} actions", aActions.Actions.size());
                    actions.insert(actions.end(), aActions.Actions.begin(), aActions.Actions.end());
                },
                [&](const NewGameRequest& aNewGame) {
                    GameInstanceID gameID = createGameInstance(aNewGame);
                    spdlog::info("Created game {}", gameID);
                    mServer.EnqueueResponse(new NetworkEvent<NetworkResponsePayload>{
                        .Type     = PacketType::NewGame,
                        .PlayerID = 0,
                        .Payload  = NewGameResponse{.GameID = gameID}});
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
            mServer.ConsumeNetworkResponses();
            mServer.Poll();
        }
    }};

    while (mRunning) {
        auto                         t  = clock_type::now();
        std::chrono::duration<float> dt = (t - prevTime);
        prevTime                        = t;

        ConsumeNetworkRequests();
        // Update each game instance independently
        for (auto& [gameId, registry] : mGameInstances) {
            AdvanceSimulation(registry, dt.count());
        }
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

    StartGameInstance(registry, gameID);
    return gameID;
}
