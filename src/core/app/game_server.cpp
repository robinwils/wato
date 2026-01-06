#include "core/app/game_server.hpp"

#include <bx/bx.h>
#include <fmt/ranges.h>
#include <spdlog/spdlog.h>

#include <thread>

#include "components/player.hpp"
#include "components/spawner.hpp"
#include "core/net/net.hpp"
#include "core/snapshot.hpp"
#include "core/sys/log.hpp"
#include "core/types.hpp"
#include "input/action.hpp"
#include "registry/registry.hpp"
#include "systems/action.hpp"
#include "systems/ai.hpp"
#include "systems/collision.hpp"
#include "systems/health.hpp"
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
    spawnPlayers(aRegistry);

    auto& fixedExec = aRegistry.ctx().get<FixedSystemExecutor>();

    fixedExec.Register<NetworkSyncSystem<ENetServer>>();
    fixedExec.Register<HealthSystem>();
    fixedExec.Register<CollisionSystem>();
    fixedExec.Register<PhysicsSystem>();
    fixedExec.Register<TowerBuiltSystem>();
    fixedExec.Register<RigidBodiesUpdateSystem>();
    fixedExec.Register<ProjectileSystem>();
    fixedExec.Register<TowerAttackSystem>();
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

                    for (auto&& [entity, player] : mGameInstances[gameID].view<Player>().each()) {
                        mLogger->debug(
                            "Sending network response for game {}, player {}, player {}",
                            gameID,
                            player.ID,
                            entity);
                        mServer.EnqueueResponse(new NetworkResponse{
                            .Type     = PacketType::NewGame,
                            .PlayerID = player.ID,
                            .Tick     = 0,
                            .Payload  = NewGameResponse{.GameID = gameID, .PlayerEntity = entity}});
                    }
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
                if (!aEvent->Archive(archive)) {
                    mLogger->error("could not archive response {}", *aEvent);
                }

                mLogger->trace("sending {}", *aEvent);
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

void GameServer::spawnPlayers(Registry& aRegistry)
{
    auto& colliderToEntity = aRegistry.ctx().get<ColliderEntityMap>();
    auto& graph            = aRegistry.ctx().get<Graph>();

    auto player = aRegistry.create();
    // TODO: ID should be something coming from outside (menu, DB, etc...)
    aRegistry.emplace<Player>(player, 0u);
    aRegistry.emplace<Name>(player, "stion");
    aRegistry.emplace<Health>(player, 100.0f);

    WATO_INFO(aRegistry, "server player {} created", player);

    auto& pTransform = aRegistry.emplace<Transform3D>(player, glm::vec3(2.0f, 0.004f, 2.0f));
    aRegistry.emplace<RigidBody>(
        player,
        RigidBody{
            .Params =
                RigidBodyParams{
                    .Type           = rp3d::BodyType::STATIC,
                    .Velocity       = 0.0f,
                    .Direction      = glm::vec3(0.0f),
                    .GravityEnabled = false,
                },
        });
    auto& c = aRegistry.emplace<Collider>(
        player,
        Collider{
            .Params =
                ColliderParams{
                    .CollisionCategoryBits = Category::Base,
                    .CollideWithMaskBits   = Category::Terrain | Category::Entities,
                    .IsTrigger             = true,
                    .ShapeParams =
                        BoxShapeParams{
                            .HalfExtents = GraphCell(1, 1).ToWorld() * 0.5f,
                        },
                },
        });
    colliderToEntity[c.Handle] = player;

    graph.ComputePaths(GraphCell::FromWorldPoint(pTransform.Position));
}

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
