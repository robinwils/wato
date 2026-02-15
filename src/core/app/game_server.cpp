#include "core/app/game_server.hpp"

#include <bx/bx.h>
#include <fmt/ranges.h>
#include <spdlog/spdlog.h>

#include <thread>

#include "components/player.hpp"
#include "components/spawner.hpp"
#include "core/net/net.hpp"
#include "core/net/pocketbase.hpp"
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
    if (!mServer.IsInit()) {
        return;
    }

    mFrameExecutor.Register<SimulationSystem>();

    mPBClient.Subscribe<GameRecord>(
        "game/*",
        [this](const std::optional<PBSSE<GameRecord>>& aRecord, const std::string& aError) {
            if (aRecord) {
                mPBGameChan.Send(new PBSSE<GameRecord>(*aRecord));
            }
        },
        "id,players");
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
    // init groups when registry is empty to get the most performance
    aRegistry.group<Player>(entt::get<Health>, entt::exclude<Eliminated>);

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
    struct RequestVisitor {
        std::unordered_map<GameInstanceID, Registry>* GameInstances;
        spdlog::logger*                               Log;
        NetworkRequest*                               Event;

        void operator()(const SyncPayload& aReq) const
        {
            if (!GameInstances->contains(aReq.GameID)) {
                Log->warn("got event for non existing game {}", aReq.GameID);
                return;
            }

            Registry& registry = (*GameInstances)[aReq.GameID];
            if (IsPlayerEliminated(registry, Event->PlayerID)) {
                Log->debug("got event for eliminated player {}", Event->PlayerID);
                return;
            }

            const ActionsType& incoming      = aReq.State.Actions;
            auto&              taggedActions = registry.ctx().get<TaggedActionsType>();

            Log->debug(
                "got {} actions from player {}: {}",
                incoming.size(),
                Event->PlayerID,
                incoming);
            for (const auto& action : incoming) {
                taggedActions.push_back({Event->PlayerID, action});
            }
        }

        void operator()(const AuthRequest&) const {}
        void operator()(const std::monostate&) const {}
    };

    mServer.ConsumeNetworkRequests([&](NetworkRequest* aEvent) {
        std::visit(RequestVisitor{&mGameInstances, mLogger.get(), aEvent}, aEvent->Payload);
    });

    mPBGameChan.Drain([&](PBSSE<GameRecord>* aEvent) {
        if (aEvent->action == "create") {
            mLogger->info("Created game {}", aEvent->record.id);

            auto gameID = GameIDFromHexString(aEvent->record.id);

            if (!gameID) {
                mLogger->error("Invalid game ID from server: '{}'", aEvent->record.id);
            }
            createGameInstance(*gameID);

            for (auto&& [entity, player] : mGameInstances[*gameID].view<Player>().each()) {
                mLogger->debug(
                    "Sending network response for game {}, player {}, player {}",
                    *gameID,
                    player.ID,
                    entity);
                mServer.EnqueueResponse(new NetworkResponse{
                    .Type     = PacketType::NewGame,
                    .PlayerID = player.ID,
                    .Tick     = 0,
                    .Payload  = NewGameResponse{.GameID = *gameID, .PlayerEntity = entity}});
            }
        }
    });
}

int GameServer::Run(tf::Executor& aExecutor)
{
    if (!mServer.IsInit()) {
        return 1;
    }
    mLogger->debug("running game server");
    auto prevTime = clock_type::now();
    mRunning      = true;

    aExecutor.silent_async([&]() {
        while (mRunning) {
            mServer.ProcessAuthResults();
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
    aRegistry.emplace<DisplayName>(player, "stion");
    aRegistry.emplace<Health>(player, 10.0f);

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

void GameServer::createGameInstance(GameInstanceID aGameID)
{
    Registry& registry = mGameInstances[aGameID];

    registry.ctx().emplace<Logger>(mLogger);
    registry.ctx().emplace<ENetServer&>(mServer);
    registry.ctx().emplace_as<std::vector<PlayerID>>("ranking"_hs);

    StartGameInstance(registry, aGameID, true);
}

GameInstanceID GameServer::createGameInstance()
{
    GameInstanceID gameID;
    do {
        gameID = GenerateGameInstanceID();
    } while (mGameInstances.contains(gameID));

    Registry& registry = mGameInstances[gameID];

    registry.ctx().emplace<Logger>(mLogger);
    registry.ctx().emplace<ENetServer&>(mServer);
    registry.ctx().emplace_as<std::vector<PlayerID>>("ranking"_hs);

    StartGameInstance(registry, gameID, true);

    return gameID;
}
