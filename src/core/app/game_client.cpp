#include "core/app/game_client.hpp"

#include <bx/bx.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <entt/core/fwd.hpp>
#include <entt/signal/dispatcher.hpp>
#include <memory>
#include <span>

#include "components/game.hpp"
#include "components/imgui.hpp"
#include "core/menu/menu_state.hpp"
#include "core/net/enet_client.hpp"
#include "core/net/net.hpp"
#include "core/net/network_events.hpp"
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

using namespace std::literals::chrono_literals;
using namespace entt::literals;

void GameClient::Init()
{
    Application::Init();

    spdlog::set_default_logger(mLogger);

    auto& window    = mRegistry.ctx().get<WatoWindow>();
    auto& renderer  = mRegistry.ctx().get<BgfxRenderer&>();
    auto& netClient = mRegistry.ctx().get<ENetClient>();

    window.Init();
    renderer.Init(window);
    netClient.Init();

    LoadResources(mRegistry);

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

    // TODO: We need to handle NewGame in main menu, separate game instance
    // lifetime from FixedTime system ?
    // mMenuExecutor.Register<NetworkResponseSystem>();
}

int GameClient::Run(tf::Executor& aExecutor)
{
    auto& window    = mRegistry.ctx().get<WatoWindow&>();
    auto& renderer  = mRegistry.ctx().get<BgfxRenderer&>();
    auto& netClient = mRegistry.ctx().get<ENetClient&>();
    auto  prevTime  = clock_type::now();

    std::thread networkThreadHandle([this]() { networkThread(); });
    networkThreadHandle.detach();

    if (!netClient.Connect()) {
        throw std::runtime_error("No available peers for initiating an ENet connection.");
    }
    WATO_DBG(mRegistry, "connected to server");

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

        switch (mRegistry.ctx().get<MenuState>()) {
            case MenuState::MainMenu:
                mMenuExecutor.Update(frameTime.count(), &mRegistry);
                break;
            case MenuState::InGame:
                if (mRegistry.ctx().contains<GameInstance>()) {
                    mFrameExecutor.Update(frameTime.count(), &mRegistry);
                }
                break;
            case MenuState::EndGame:
                break;
        }

        renderer.Render();
        input.PrevKeyboardState = input.KeyboardState;
        input.PrevMouseState    = input.MouseState;
        input.MouseState.Scroll = glm::dvec2(0.0);
    }

    StopGameInstance(mRegistry);
    netClient.Disconnect();
    mDiscTimerStart.emplace(clock_type::now());

    return 0;
}

void GameClient::networkThread()
{
    auto& netClient = mRegistry.ctx().get<ENetClient&>();
    while (netClient.Running()) {
        if (mDiscTimerStart) {
            if (clock_type::now() - *mDiscTimerStart > 3s) {
                netClient.ForceDisconnect();
            }
        }

        netClient.ConsumeNetworkRequests([&](NetworkRequest* aEvent) {
            // write header
            BitOutputArchive archive;

            aEvent->Archive(archive);

            netClient.Send(archive.Bytes());
        });
        netClient.Poll();
    }
}

void GameClient::spawnPlayerAndCamera()
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
        glm::vec3(3.0f, 2.0f, 3.0f),
        glm::identity<glm::quat>(),
        glm::vec3(1.0f));
    mRegistry.emplace<ImguiDrawable>(camera, "Camera");
}

void GameClient::prepareGridPreview()
{
    const auto& graph = mRegistry.ctx().get<Graph>();

    GraphCell::size_type numVertsX = graph.Width() + 1;
    GraphCell::size_type numVertsY = graph.Height() + 1;

    std::vector<PositionVertex> vertices;
    std::vector<uint16_t>       indices;

    for (GraphCell::size_type i = 0; i < numVertsY; ++i) {
        for (GraphCell::size_type j = 0; j < numVertsX; ++j) {
            GraphCell cell(j, i);
            vertices.emplace_back(cell.ToWorld());
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
    auto& renderer = mRegistry.ctx().get<BgfxRenderer&>();
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

void GameClient::OnGameInstanceCreated(Registry& aRegistry)
{
    auto& fixedExec = aRegistry.ctx().get<FixedSystemExecutor>();

    spawnPlayerAndCamera();
    prepareGridPreview();

    fixedExec.Register<NetworkSyncSystem<ENetClient>>();
    fixedExec.Register<PhysicsSystem>();
    fixedExec.Register<RigidBodiesUpdateSystem>();
    fixedExec.Register<TowerBuiltSystem>();
    fixedExec.Register<DeterministicActionSystem>();
    fixedExec.Register<NetworkResponseSystem>();
}

void GameClient::consumeNetworkResponses()
{
    auto& netClient = mRegistry.ctx().get<ENetClient>();

    auto* dispatcher = mRegistry.ctx().find<entt::dispatcher>();

    netClient.ConsumeNetworkResponses([&](NetworkResponse* aEvent) {
        std::visit(
            VariantVisitor{
                [&](const ConnectedResponse&) {
                    WATO_INFO(mRegistry, "got connected response");

                    // TODO: should be triggered by player input (menu, matchmaking,...)
                    netClient.EnqueueRequest(new NetworkRequest{
                        .Type     = PacketType::NewGame,
                        .PlayerID = 0,
                        .Tick     = 0,
                        .Payload  = NewGameRequest{.PlayerAID = 0},
                    });
                },
                [&](const NewGameResponse& aResp) {
                    StartGameInstance(mRegistry, aResp.GameID, false);
                    WATO_INFO(mRegistry, "game {} created", aResp.GameID);

                    if (dispatcher) {
                        dispatcher->enqueue<NewGameEvent>(
                            NewGameEvent{.Reg = &mRegistry, .Response = std::move(aResp)});
                    }
                },
                [&](SyncPayload& aResp) {
                    if (dispatcher) {
                        dispatcher->enqueue<SyncPayloadEvent>(
                            SyncPayloadEvent{.Reg = &mRegistry, .Payload = std::move(aResp)});
                    }
                },
                [&](const RigidBodyUpdateResponse& aUpdate) {
                    if (dispatcher) {
                        dispatcher->enqueue<RigidBodyUpdateEvent>(
                            RigidBodyUpdateEvent{.Reg = &mRegistry, .Response = aUpdate});
                    }
                },
                [&](const HealthUpdateResponse& aUpdate) {
                    if (dispatcher) {
                        dispatcher->enqueue<HealthUpdateEvent>(
                            HealthUpdateEvent{.Reg = &mRegistry, .Response = aUpdate});
                    }
                },
                [&](const PlayerEliminatedResponse& aResp) {
                    mRegistry.ctx().insert_or_assign("ranking"_hs, aResp.Ranking);
                    mRegistry.ctx().insert_or_assign<MenuState>(MenuState::EndGame);
                },
                [&](const GameEndResponse& aResp) {
                    mRegistry.ctx().insert_or_assign("ranking"_hs, aResp.Ranking);
                    mRegistry.ctx().insert_or_assign<MenuState>(MenuState::EndGame);
                },
                [&](const std::monostate) {},
            },
            aEvent->Payload);
    });
}
