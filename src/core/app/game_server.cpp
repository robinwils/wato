#include "core/app/game_server.hpp"

#include <bx/bx.h>
#include <fmt/ranges.h>
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
    mSystemsFT.push_back(AiSystem::MakeDelegate(mAiSystem));
    mSystemsFT.push_back(ServerActionSystem::MakeDelegate(mActionSystem));
}

GameServer::~GameServer()
{
    spdlog::trace("destroying game server");
    spdlog::trace("deleting {} worlds", mGameInstances.size());
    for (auto& i : mGameInstances) {
        auto& p = i.second.ctx().get<Physics>();
        spdlog::trace("got world {}", fmt::ptr(p.World()));
    }
}

void GameServer::ConsumeNetworkRequests()
{
    while (const NetworkEvent<NetworkRequestPayload>* ev = mServer.Queue().pop()) {
        std::visit(
            VariantVisitor{
                [&](const SyncPayload& aReq) {
                    if (!mGameInstances.contains(aReq.GameID)) {
                        spdlog::warn("got event for non existing game {}", aReq.GameID);
                        return;
                    }

                    const ActionsType& incoming = aReq.State.Actions;
                    Registry&          registry = mGameInstances[aReq.GameID];
                    auto& actions = registry.ctx().get<GameStateBuffer&>().Latest().Actions;

                    spdlog::debug("got {} actions: {}", incoming.size(), incoming);
                    actions.insert(actions.end(), incoming.begin(), incoming.end());
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

int GameServer::Run(tf::Executor& aExecutor)
{
    spdlog::debug("running game server");
    auto prevTime = clock_type::now();
    mRunning      = true;

    aExecutor.silent_async([&]() {
        while (mRunning) {
            mServer.ConsumeNetworkResponses();
            mServer.Poll();
        }
    });

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

    for (auto& [gameId, registry] : mGameInstances) {
        StopGameInstance(registry);
    }

    return 0;
}

void GameServer::Stop() { mRunning = false; }

GameInstanceID GameServer::createGameInstance(const NewGameRequest& aNewGame)
{
    GameInstanceID gameID;
    do {
        gameID = GenerateGameInstanceID();
    } while (mGameInstances.contains(gameID));

    Registry& registry = mGameInstances[gameID];

    StartGameInstance(registry, gameID, true);
    return gameID;
}
