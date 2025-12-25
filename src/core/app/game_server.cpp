#include "core/app/game_server.hpp"

#include <bx/bx.h>
#include <fmt/ranges.h>
#include <spdlog/spdlog.h>

#include <thread>

#include "core/net/net.hpp"
#include "core/snapshot.hpp"
#include "core/types.hpp"
#include "input/action.hpp"
#include "registry/registry.hpp"
#include "systems/action.hpp"
#include "systems/ai.hpp"
#include "systems/physics.hpp"
#include "systems/projectile.hpp"
#include "systems/rigid_bodies_update.hpp"
#include "systems/sync.hpp"
#include "systems/system.hpp"
#include "systems/tower_attack.hpp"
#include "systems/tower_built.hpp"

void GameServer::Init()
{
    Application::Init();

    mServer.Init();

    mFrameExecutor.Register<SimulationSystem>();
}

GameServer::~GameServer()
{
    mLogger->trace("destroying game server");
    mLogger->trace("deleting {} worlds", mGameInstances.size());
    for (auto& i : mGameInstances) {
        auto& p = i.second.ctx().get<Physics>();
        mLogger->trace("got world {}", fmt::ptr(p.World()));
    }
}

void GameServer::OnGameInstanceCreated(Registry& aRegistry)
{
    auto& fixedExec = aRegistry.ctx().get<FixedSystemExecutor>();

    fixedExec.Register<NetworkSyncSystem<ENetServer>>();
    fixedExec.Register<PhysicsSystem>();
    fixedExec.Register<RigidBodiesUpdateSystem>();
    fixedExec.Register<ProjectileSystem>();
    fixedExec.Register<TowerAttackSystem>();
    fixedExec.Register<TowerBuiltSystem>();
    fixedExec.Register<AiSystem>();
    fixedExec.Register<ServerActionSystem>();
}

void GameServer::ConsumeNetworkRequests()
{
    mServer.ConsumeNetworkRequests([&](NetworkRequest* aEvent) {
        std::visit(
            VariantVisitor{
                [&](const SyncPayload& aReq) {
                    if (!mGameInstances.contains(aReq.GameID)) {
                        mLogger->warn("got event for non existing game {}", aReq.GameID);
                        return;
                    }

                    const ActionsType& incoming = aReq.State.Actions;
                    Registry&          registry = mGameInstances[aReq.GameID];
                    auto& actions = registry.ctx().get<GameStateBuffer&>().Latest().Actions;

                    mLogger->debug("got {} actions: {}", incoming.size(), incoming);
                    actions.insert(actions.end(), incoming.begin(), incoming.end());
                },
                [&](const NewGameRequest& aNewGame) {
                    GameInstanceID gameID = createGameInstance(aNewGame);
                    mLogger->info("Created game {}", gameID);
                    mServer.EnqueueResponse(new NetworkResponse{
                        .Type     = PacketType::NewGame,
                        .PlayerID = 0,
                        .Tick     = 0,
                        .Payload  = NewGameResponse{.GameID = gameID}});
                },
                [&](const std::monostate&) {}},
            aEvent->Payload);
    });
}

int GameServer::Run(tf::Executor& aExecutor)
{
    mLogger->debug("running game server");
    auto prevTime = clock_type::now();
    mRunning      = true;

    aExecutor.silent_async([&]() {
        while (mRunning) {
            mServer.ConsumeNetworkResponses([&](NetworkResponse* aEvent) {
                BitOutputArchive archive;
                aEvent->Archive(archive);

                if (!mServer.Send(aEvent->PlayerID, archive.Bytes())) {
                    mLogger->error("player {} is not connected", aEvent->PlayerID);
                    mRunning = false;
                }
            });
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
            mFrameExecutor.Update(dt.count(), &registry);
        }
    }

    for (auto& [gameId, registry] : mGameInstances) {
        StopGameInstance(registry);
    }

    return 0;
}

void GameServer::Stop() { mRunning = false; }

GameInstanceID GameServer::createGameInstance(const NewGameRequest&)
{
    GameInstanceID gameID;
    do {
        gameID = GenerateGameInstanceID();
    } while (mGameInstances.contains(gameID));

    Registry& registry = mGameInstances[gameID];

    registry.ctx().emplace<Logger>(mLogger);
    registry.ctx().emplace<ENetServer&>(mServer);

    StartGameInstance(registry, gameID, true);

    return gameID;
}
