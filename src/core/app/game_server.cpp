#include "core/app/game_server.hpp"

#include <bx/bx.h>
#include <fmt/ranges.h>
#include <sodium/runtime.h>
#include <spdlog/spdlog.h>

#include <glaze/glaze.hpp>
#include <thread>

#include "components/game.hpp"
#include "components/health.hpp"
#include "components/player.hpp"
#include "components/spawner.hpp"
#include "components/transform3d.hpp"
#include "core/net/net.hpp"
#include "core/net/pocketbase.hpp"
#include "core/physics/physics.hpp"
#include "core/snapshot.hpp"
#include "core/sys/log.hpp"
#include "core/sys/signal.hpp"
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

    if (!mAdminEmail.empty() && !mAdminPassword.empty()) {
        auto loginResult = mPBClient.LoginSuperuserSync(mAdminEmail, mAdminPassword);
        if (!loginResult) {
            mLogger->error("superuser login failed: {}", loginResult.error().Message);
            return;
        }
        mLogger->info("superuser login successful");
    } else {
        mLogger->warn("no admin credentials, will not be able to use pocketbase");
    }

    mFrameExecutor.Register<SimulationSystem>();

    mPBClient.Subscribe<GameRecord>("game/*", mPBGameChan, "id,players,created", [this]() {
        if (mLastGameTimestamp.empty()) return;
        mPBClient.GetGamesSince(
            mLastGameTimestamp,
            [this](std::expected<GameRecordList, PBError> aResult) {
                if (!aResult) {
                    mLogger->error("GetGamesSince failed: {}", aResult.error().Message);
                    return;
                }
                for (const auto& r : aResult->items) {
                    mPBGameChan.Send(new PBSSE<GameRecord>{.action = "create", .record = r});
                }
            });
    });

    // We are taking advantage of TLS here. Publishing the server's public key in PocketBase
    // game_servers collection allows the client to GET it securely through HTTP + TLS (if
    // configured), keeping the ENet handshake minimal.
    auto [ip, port]    = mServer.IPAndPort();
    std::string pubKey = mServer.PublicKey();

    if (pubKey.empty()) {
        mLogger->warn("could not encode server public key");
    }

    auto r = mPBClient.RegisterGameServerSync(ip, port, pubKey, sodium_runtime_has_aesni());
    if (!r) {
        mLogger->error("could not register game server: {}", r.error().Message);
    }
}

GameServer::~GameServer()
{
    mLogger->trace("destroying game server");
    mLogger->trace("deleting {} worlds", mGameInstances.size());
    for (auto& i : mGameInstances) {
        auto& p = GetSingletonComponent<Physics>(i.second);
        mLogger->trace("got world {}", fmt::ptr(p.World()));
    }
}

std::vector<PlayerInitData> GameServer::StartGameInstance(
    Registry&             aRegistry,
    const GameInstanceID  aGameID,
    std::vector<PlayerID> aPlayerIDs)
{
    Application::StartGameInstance(aRegistry, aGameID);

    aRegistry.ctx().emplace<PlayerGraphMap>();
    // aRegistry.ctx().emplace<ActionContextStack>().back().State = ActionContext::State::Server;
    aRegistry.ctx().emplace<PocketBaseClient&>(mPBClient);
    // init groups when registry is empty to get the most performance
    aRegistry.group<Player>(entt::get<Health>, entt::exclude<Eliminated>);

    auto playerInitData = spawnPlayers(aRegistry, aPlayerIDs);

    auto& fixedExec = GetSingletonComponent<FixedSystemExecutor>(aRegistry);

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

    return playerInitData;
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
            auto&              taggedActions = GetSingletonComponent<TaggedActionsType>(registry);

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
            mLastGameTimestamp = aEvent->record.created;
            mLogger->info("Created game {}", aEvent->record.id);

            auto gameID = GameIDFromHexString(aEvent->record.id);

            if (!gameID) {
                mLogger->error("Invalid game ID from server: '{}'", aEvent->record.id);
                return;
            }

            std::vector<PlayerID> playerIDs;
            for (const std::string& pHexID : aEvent->record.players) {
                auto pID = PlayerIDFromHexString(pHexID);
                if (!pID) {
                    mLogger->error("Invalid game ID from server: '{}'", aEvent->record.id);
                    return;
                }

                playerIDs.push_back(*pID);
            }
            auto playerInitData = createGameInstance(*gameID, playerIDs);

            mGameInstances[*gameID].ctx().get<GameInstance&>().Record = aEvent->record.id;

            for (const auto& p : playerInitData) {
                mLogger->debug("Sending network response for game {}, player {}", *gameID, p.ID);
                mServer.EnqueueResponse(new NetworkResponse{
                    .Type     = PacketType::NewGame,
                    .PlayerID = p.ID,
                    .Tick     = 0,
                    .Payload  = NewGameResponse{
                         .GameID       = *gameID,
                         .YourPlayerID = p.ID,
                         .Players      = playerInitData,
                    }});
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

    constexpr auto kTargetFrameTime = std::chrono::duration<double>(kTimeStep);

    while (mRunning) {
        if (gShutdownRequested.load()) {
            Stop();
            break;
        }

        auto                         t  = clock_type::now();
        std::chrono::duration<float> dt = (t - prevTime);
        prevTime                        = t;

        mPBClient.Update();
        ConsumeNetworkRequests();
        // Update each game instance independently
        for (auto& [gameId, registry] : mGameInstances) {
            mFrameExecutor.Update(dt.count(), &registry);
        }

        auto elapsed   = clock_type::now() - t;
        auto remaining = kTargetFrameTime - elapsed;
        if (remaining > std::chrono::duration<double>(0)) {
            std::this_thread::sleep_for(remaining);
        }
    }

    for (auto& [gameId, registry] : mGameInstances) {
        StopGameInstance(registry);
    }

    return 0;
}

void GameServer::Stop() { mRunning = false; }

std::vector<PlayerInitData> GameServer::spawnPlayers(
    Registry&                 aRegistry,
    std::span<const PlayerID> aPlayerIDs)
{
    std::vector<PlayerInitData> result;
    glm::uvec2                  size{20, 20};
    glm::vec2                   offset{0, 0};

    for (uint8_t idx = 0; idx < aPlayerIDs.size(); ++idx) {
        uint8_t  sender = idx == 0 ? uint8_t(aPlayerIDs.size()) - 1 : idx - 1;
        PlayerID id     = aPlayerIDs[idx];
        auto     player = aRegistry.create();

        aRegistry.emplace<Player>(player, id, idx);
        aRegistry.emplace<DisplayName>(player, mServer.GetAccountName(id));
        aRegistry.emplace<Health>(player, 10.0f);
        WATO_INFO(aRegistry, "server player {} created", player);

        auto& pTransform = aRegistry.emplace<Transform3D>(
            player,
            glm::vec3(2.0f + offset.x, 0.004f, 2.0f + offset.y));
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
                        .CollideWithMaskBits   = CollidesWith(
                            PlayerEntitiesCategory(sender),
                            Category::Terrain,
                            Category::Tower),
                        .IsTrigger = true,
                        .ShapeParams =
                            BoxShapeParams{
                                .HalfExtents = GraphCell(1, 1).ToWorld() * 0.5f,
                            },
                    },
            });

        SpawnTerrain(aRegistry, player, size, offset);

        auto [it, inserted] = GetSingletonComponent<PlayerGraphMap&>(aRegistry).try_emplace(
            id,
            size.x * GraphCell::kCellsPerAxis,
            size.y * GraphCell::kCellsPerAxis,
            offset);

        it->second.ComputePaths(pTransform.Position);
        WATO_DBG(aRegistry, "{}", it->second);

        result.push_back(PlayerInitData{
            .ID             = id,
            .ServerEntity   = player,
            .Health         = 10.0f,
            .DisplayName    = mServer.GetAccountName(id),
            .Position       = pTransform.Position,
            .MapSize        = size,
            .MapWorldOffset = offset,
        });

        offset.x += float(size.x) + 5.0f;
    }

    return result;
}

std::vector<PlayerInitData> GameServer::createGameInstance(
    GameInstanceID        aGameID,
    std::vector<PlayerID> aPlayerIDs)
{
    if (mGameInstances.contains(aGameID)) {
        mLogger->warn("game {} already exists, skipping duplicate", aGameID);
        return {};
    }

    Registry& registry = mGameInstances[aGameID];

    registry.ctx().emplace<Logger>(mLogger);
    registry.ctx().emplace<ENetServer&>(mServer);
    registry.ctx().emplace<const GameplayDef&>(mGameplayDef);
    registry.ctx().emplace_as<std::vector<PlayerID>>("ranking"_hs);
    registry.ctx().emplace<TaggedActionsType>();

    return StartGameInstance(registry, aGameID, aPlayerIDs);
}
