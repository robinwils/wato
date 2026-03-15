#include "core/app/game_client.hpp"

#include <bx/bx.h>
#include <sodium/runtime.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <entt/core/fwd.hpp>
#include <entt/signal/dispatcher.hpp>
#include <memory>
#include <span>

#include "components/game.hpp"
#include "components/health.hpp"
#include "components/imgui.hpp"
#include "components/player.hpp"
#include "components/rigid_body.hpp"
#include "components/transform3d.hpp"
#include "core/crypto/key.hpp"
#include "core/gameplay_definitions.hpp"
#include "core/graph.hpp"
#include "core/menu/menu.hpp"
#include "core/net/enet_client.hpp"
#include "core/net/net.hpp"
#include "core/net/network_events.hpp"
#include "core/net/pocketbase.hpp"
#include "core/physics/physics.hpp"
#include "core/snapshot.hpp"
#include "core/sys/log.hpp"
#include "core/types.hpp"
#include "core/window.hpp"
#include "registry/game_registry.hpp"
#include "registry/registry.hpp"
#include "renderer/grid_preview_material.hpp"
#include "renderer/primitive.hpp"
#include "renderer/renderer.hpp"
#include "systems/action.hpp"
#include "systems/animation.hpp"
#include "systems/input.hpp"
#include "systems/network_response.hpp"
#include "systems/physics.hpp"
#include "systems/render.hpp"
#include "systems/rigid_bodies_update.hpp"
#include "systems/sync.hpp"
#include "systems/system.hpp"
#include "systems/tower_built.hpp"
#include "systems/ui.hpp"

using namespace std::literals::chrono_literals;
using namespace entt::literals;

void GameClient::Init()
{
    Application::Init();

    spdlog::set_default_logger(mLogger);

    auto& window    = GetSingletonComponent<WatoWindow>(mRegistry);
    auto& renderer  = GetSingletonComponent<BgfxRenderer&>(mRegistry);
    auto& netClient = GetSingletonComponent<ENetClient>(mRegistry);

    window.Init();
    renderer.Init(window);
    netClient.Init();

    // Register frame-time systems (variable delta)
    // in reversed order, entt::scheduler executes processes starting from the end
#if WATO_DEBUG
    mFrameExecutor.Register<PhysicsDebugSystem>();
#endif
    mFrameExecutor.Register<RenderSystem>();
    mFrameExecutor.Register<RenderImguiSystem>();
    mFrameExecutor.Register<SimulationSystem>();
    mFrameExecutor.Register<CameraSystem>();
    mFrameExecutor.Register<AnimationSystem>();
    mFrameExecutor.Register<RealTimeActionSystem>();
    mFrameExecutor.Register<InputSystem>();

    mMenuExecutor.Register<RenderSystem>();
    mMenuExecutor.Register<RenderImguiSystem>();
    mMenuExecutor.Register<UISystem>();

    // End-game: render world + rankings board, no simulation or input
    mEndGameExecutor.Register<RenderSystem>();
    mEndGameExecutor.Register<RenderImguiSystem>();
    mEndGameExecutor.Register<AnimationSystem>();

    // fetch ENet server public key from pocketbase which is a secure channel if TLS enabled
    auto r = mRegistry.ctx().get<PocketBaseClient&>().GetGameServer("0.0.0.0", 7777);
    if (!r) {
        mLogger->error("cannot get game server");
        return;
    }

    mLogger->debug("got server public key = {}", r->publicKey);
    mRegistry.ctx().get<ENetClient&>().SetServerPK(KeyFromB64<32>(r->publicKey));
}

int GameClient::Run(tf::Executor& aExecutor)
{
    auto& window    = GetSingletonComponent<WatoWindow&>(mRegistry);
    auto& renderer  = GetSingletonComponent<BgfxRenderer&>(mRegistry);
    auto& netClient = GetSingletonComponent<ENetClient&>(mRegistry);
    auto  prevTime  = clock_type::now();

    mNetworkThread = std::thread([this]() { networkThread(); });

    while (!window.ShouldClose()) {
        window.PollEvents();

        if (window.Resize()) {
            renderer.Resize(window);
        }

        Input&                       input     = window.GetInput();
        auto                         now       = clock_type::now();
        std::chrono::duration<float> frameTime = (now - prevTime);

        prevTime = now;

        consumeNetworkResponses();

        switch (mRegistry.ctx().get<MenuContext>().State) {
            case MenuState::Login:
            case MenuState::Register:
            case MenuState::Lobby:
                mMenuExecutor.Update(frameTime.count(), &mRegistry);
                break;
            case MenuState::InGame:
                if (mRegistry.ctx().contains<GameInstance>()) {
                    mFrameExecutor.Update(frameTime.count(), &mRegistry);
                }
                break;
            case MenuState::EndGame:
                mEndGameExecutor.Update(frameTime.count(), &mRegistry);
                break;
        }

        renderer.Render();
        input.PrevKeyboardState = input.KeyboardState;
        input.PrevMouseState    = input.MouseState;
        input.MouseState.Scroll = glm::dvec2(0.0);
    }

    netClient.Disconnect();
    mDiscTimerStart.emplace(clock_type::now());

    return 0;
}

void GameClient::networkThread()
{
    auto& netClient = GetSingletonComponent<ENetClient&>(mRegistry);
    while (netClient.Running()) {
        if (mDiscTimerStart) {
            if (clock_type::now() - *mDiscTimerStart > 3s) {
                netClient.ForceDisconnect();
            }
        }

        netClient.ConsumeNetworkRequests([&](NetworkRequest* aEvent) {
            if (aEvent->Type == PacketType::Auth) {
                netClient.ResetSession();
            }

            BitOutputArchive archive;

            aEvent->Archive(archive);

            netClient.Send(archive.Bytes());
        });
        netClient.Poll();
    }
}

void GameClient::spawnCamera(glm::vec3 aPlayerPosition)
{
    auto camera = mRegistry.create();
    mRegistry.emplace<Camera>(
        camera,
        // up, front, dir
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, -1.0f),
        glm::vec3(0.0f, -1.0f, -1.0f),
        // speed, fov, near, far
        2.5f,
        60.0f,
        0.1f,
        100.0f);
    // pos, orientation, scale
    mRegistry.emplace<Transform3D>(
        camera,
        aPlayerPosition + glm::vec3(1.0f, 2.0f, 1.0f),
        glm::identity<glm::quat>(),
        glm::vec3(1.0f));
    mRegistry.emplace<ImguiDrawable>(camera, "Camera");
}

void GameClient::prepareGridPreview(const glm::vec2& aOffset)
{
    const auto& graph = GetSingletonComponent<Graph>(mRegistry);

    GraphCell::size_type numVertsX = graph.Width() + 1;
    GraphCell::size_type numVertsY = graph.Height() + 1;

    std::vector<PositionVertex> vertices;
    std::vector<uint16_t>       indices;

    for (GraphCell::size_type i = 0; i < numVertsY; ++i) {
        for (GraphCell::size_type j = 0; j < numVertsX; ++j) {
            GraphCell cell(j, i);
            vertices.emplace_back(cell.ToWorld(aOffset));
            if (i != 0) {
                indices.push_back(SafeU16(i) * numVertsX + j);
                indices.push_back((SafeU16(i) - 1) * numVertsX + j);
            }
            if (j != 0) {
                indices.push_back(SafeU16(i) * numVertsX + j);
                indices.push_back(SafeU16(i) * numVertsX + j - 1);
            }
        }
    }

    const auto& texture = LoadResource(
        mRegistry.ctx().get<TextureCache>(),
        "grid_tex",
        graph.Width(),
        graph.Height(),
        false,
        1,
        bgfx::TextureFormat::R8,
        0,
        nullptr);

    const entt::resource<Shader>& shader = mRegistry.ctx().get<ShaderCache>()["grid"_hs];
    auto                          mat    = std::make_unique<GridPreviewMaterial>(
        shader,
        glm::vec4(graph.Width(), graph.Height(), GraphCell::kCellsPerAxis, 0),
        texture);
    auto primitive = std::make_unique<Primitive<PositionVertex>>(vertices, indices, std::move(mat));

    // Put texture in context variables because I am not sure entt:resource_cache can be updated
    // easily
    LoadResource(mRegistry.ctx().get<ModelCache>(), "grid", std::move(primitive));
    auto& renderer = GetSingletonComponent<BgfxRenderer&>(mRegistry);
    renderer.UpdateTexture2D(
        texture,
        0,
        0,
        0,
        0,
        graph.Width(),
        graph.Height(),
        std::span<const uint8_t>(graph.GridLayout().data(), graph.Width() * graph.Height()));
}

void GameClient::StartGameInstance(
    Registry&                          aRegistry,
    const GameInstanceID               aGameID,
    PlayerID                           aLocalPlayerID,
    const std::vector<PlayerInitData>& aPlayers)
{
    Application::StartGameInstance(aRegistry, aGameID);
    LoadResources(aRegistry);

    auto&     syncMap = GetSingletonComponent<EntitySyncMap>(aRegistry);
    glm::vec3 localPlayerPos{2.0f, 0.004f, 2.0f};

    for (uint8_t idx = 0; idx < aPlayers.size(); ++idx) {
        uint8_t sender = idx == 0 ? uint8_t(aPlayers.size()) - 1 : idx - 1;

        const PlayerInitData& p = aPlayers[idx];

        // Create player entity
        auto  player    = aRegistry.create();
        auto& playerCmp = aRegistry.emplace<Player>(player, p.ID, idx);
        aRegistry.emplace<DisplayName>(player, p.DisplayName);
        aRegistry.emplace<Health>(player, p.Health);
        aRegistry.emplace<Transform3D>(player, p.Position);
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
        aRegistry.emplace<Collider>(
            player,
            Collider{
                .Params =
                    ColliderParams{
                        .CollisionCategoryBits = Category::Base,
                        .CollideWithMaskBits   = CollidesWith(
                            PlayerEntitiesCategory(sender), Category::Tower),
                        .IsTrigger             = true,
                        .ShapeParams =
                            BoxShapeParams{
                                .HalfExtents = GraphCell(1, 1).ToWorld() * 0.5f,
                            },
                    },
            });
        SpawnTerrain(aRegistry, player, p.MapSize, p.MapWorldOffset);

        syncMap.insert_or_assign(p.ServerEntity, player);
        WATO_INFO(
            aRegistry,
            "created player {} (server {}) for player ID {}",
            player,
            p.ServerEntity,
            p.ID);

        if (p.ID == aLocalPlayerID) {
            localPlayerPos = p.Position;
            aRegistry.ctx().emplace_as<Player>("player"_hs, playerCmp);

            // Create graph for local player (used for grid preview)
            aRegistry.ctx().emplace<Graph>(
                p.MapSize.x * GraphCell::kCellsPerAxis,
                p.MapSize.y * GraphCell::kCellsPerAxis,
                p.MapWorldOffset);

            prepareGridPreview(p.MapWorldOffset);
        }
    }

    spawnCamera(localPlayerPos);

    aRegistry.ctx().emplace<const Input*>(&mRegistry.ctx().get<WatoWindow>().GetInput());
    aRegistry.ctx().emplace<const GameplayDef&>(mGameplayDef);
    aRegistry.ctx().emplace<ActionContextStack>();

    auto& fixedExec = GetSingletonComponent<FixedSystemExecutor>(aRegistry);

    fixedExec.Register<NetworkSyncSystem<ENetClient>>();
    fixedExec.Register<PhysicsSystem>();
    fixedExec.Register<RigidBodiesUpdateSystem>();
    fixedExec.Register<TowerBuiltSystem>();
    fixedExec.Register<DeterministicActionSystem>();
    fixedExec.Register<NetworkResponseSystem>();
}

void GameClient::SendAuthRequest()
{
    auto& pbase     = mRegistry.ctx().get<PocketBaseClient>();
    auto& netClient = mRegistry.ctx().get<ENetClient&>();

    auto* req = new NetworkRequest{
        .Type     = PacketType::Auth,
        .PlayerID = 0,
        .Tick     = 0,
        .Payload =
            AuthRequest{
                .Token     = pbase.Token,
                .HasAESNI  = sodium_runtime_has_aesni() != 0,
                .PublicKey = netClient.RawPublicKey()},
    };
    netClient.EnqueueRequest(req);
}

void GameClient::consumeNetworkResponses()
{
    struct NetworkResponseVisitor {
        Registry*         Reg;
        entt::dispatcher* Dispatcher;
        GameClient*       Client;

        void operator()(const ConnectedResponse&) const
        {
            WATO_INFO(*Reg, "got connected response");

            Client->SendAuthRequest();
        }

        void operator()(const ErrorResponse& aResp) const
        {
            if (aResp.Error == ServerError::HandshakeOpenSeal) {
                WATO_ERR(*Reg, "got error response");
                auto r = Reg->ctx().get<PocketBaseClient&>().GetGameServer("0.0.0.0", 7777);
                if (!r) {
                    WATO_ERR(*Reg, "cannot get game server");
                    return;
                }

                WATO_DBG(*Reg, "got server public key = {}", r->publicKey);
                Reg->ctx().get<ENetClient&>().SetServerPK(KeyFromB64<32>(r->publicKey));
                Client->SendAuthRequest();
            }
        }

        void operator()(const NewGameResponse& aResp) const
        {
            Client->StartGameInstance(*Reg, aResp.GameID, aResp.YourPlayerID, aResp.Players);
            Reg->ctx().get<MenuContext&>().State = MenuState::InGame;
            WATO_INFO(*Reg, "game {} created with {} players", aResp.GameID, aResp.Players.size());
        }

        void operator()(SyncPayload& aResp) const
        {
            Dispatcher->enqueue<SyncPayloadEvent>(
                SyncPayloadEvent{.Reg = Reg, .Payload = std::move(aResp)});
        }

        void operator()(const RigidBodyUpdateResponse& aUpdate) const
        {
            Dispatcher->enqueue<RigidBodyUpdateEvent>(
                RigidBodyUpdateEvent{.Reg = Reg, .Response = aUpdate});
        }

        void operator()(const HealthUpdateResponse& aUpdate) const
        {
            Dispatcher->enqueue<HealthUpdateEvent>(
                HealthUpdateEvent{.Reg = Reg, .Response = aUpdate});
        }

        void operator()(const PlayerEliminatedResponse& aResp) const
        {
            Reg->ctx().insert_or_assign("ranking"_hs, aResp.Ranking);
            WATO_DBG(*Reg, "got player eliminated response ranking {}", aResp.Ranking);
            if (aResp.PlayerID == Reg->ctx().get<Player>("player"_hs).ID) {
                Reg->ctx().get<MenuContext&>().State = MenuState::EndGame;
            }
        }

        void operator()(const GameEndResponse& aResp) const
        {
            Reg->ctx().insert_or_assign("ranking"_hs, aResp.Ranking);
            WATO_DBG(*Reg, "got game end response ranking {}", aResp.Ranking);
            Reg->ctx().get<MenuContext&>().State = MenuState::EndGame;
        }

        void operator()(const AuthResponse& aResp) const
        {
            if (aResp.Success) {
                WATO_INFO(*Reg, "authenticated as player {}", aResp.ID);
            } else {
                WATO_ERR(*Reg, "authentication failed");
            }
        }

        void operator()(std::monostate) const {}
    };

    auto& netClient  = GetSingletonComponent<ENetClient>(mRegistry);
    auto& dispatcher = mRegistry.ctx().get<entt::dispatcher>("net_dispatcher"_hs);

    NetworkResponseVisitor visitor{&mRegistry, &dispatcher, this};

    netClient.ConsumeNetworkResponses(
        [&](NetworkResponse* aEvent) { std::visit(visitor, aEvent->Payload); });

    mRegistry.ctx().get<PocketBaseClient>().Update();
}
