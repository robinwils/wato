#include "core/app/game_client.hpp"

#include <bx/bx.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <entt/core/fwd.hpp>
#include <memory>

#include "components/game.hpp"
#include "components/imgui.hpp"
#include "core/net/enet_client.hpp"
#include "core/net/net.hpp"
#include "core/snapshot.hpp"
#include "core/types.hpp"
#include "core/window.hpp"
#include "registry/game_registry.hpp"
#include "registry/registry.hpp"
#include "renderer/grid_preview_material.hpp"
#include "renderer/primitive.hpp"
#include "renderer/renderer.hpp"
#include "systems/render.hpp"
#include "systems/system.hpp"

using namespace std::literals::chrono_literals;
using namespace entt::literals;

void GameClient::Init()
{
    Application::Init();

    spdlog::set_default_logger(mLogger);

    auto& window    = mRegistry.ctx().get<WatoWindow>();
    auto& renderer  = mRegistry.ctx().get<Renderer>();
    auto& netClient = mRegistry.ctx().get<ENetClient>();

    window.Init();
    renderer.Init(window);
    netClient.Init();

    entt::organizer organizerFixedTime;
    LoadResources(mRegistry);
    mSystems.push_back(RenderImguiSystem::MakeDelegate(mRenderImguiSystem));
    mSystems.push_back(InputSystem::MakeDelegate(mInputSystem));
    mSystems.push_back(RealTimeActionSystem::MakeDelegate(mRTActionSystem));
    mSystems.push_back(CameraSystem::MakeDelegate(mCameraSystem));
    mSystems.push_back(RenderSystem::MakeDelegate(mRenderSystem));

#if WATO_DEBUG
    mSystems.push_back(PhysicsDebugSystem::MakeDelegate(mPhysicsDbgSystem));
#endif

    mSystemsFT.push_back(DeterministicActionSystem::MakeDelegate(mFTActionSystem));
    mSystemsFT.push_back(TowerBuiltSystem::MakeDelegate(mTowerBuiltSystem));
    mSystemsFT.push_back(RigidBodiesUpdateSystem::MakeDelegate(mRBUpdatesSystem));
    mSystemsFT.push_back(AnimationSystem::MakeDelegate(mAnimationSystem));
    mSystemsFT.push_back(PhysicsSystem::MakeDelegate(mPhysicsSystem));
    mSystemsFT.push_back(NetworkSyncSystem<ENetClient>::MakeDelegate(mNetworkSyncSystem));

    auto graph = organizerFixedTime.graph();
    WATO_INFO(mRegistry, "graph size: {}", graph.size());
    // TODO: use these tasks
    std::unordered_map<std::string, tf::Task> tasks;

    for (auto&& v : graph) {
        v.prepare(mRegistry);
        tasks[v.name()] = mTaskflow.emplace([&]() {
            typename entt::organizer::function_type* cb = v.callback();
            cb(v.data(), mRegistry);
        });
    }

    for (auto&& v : graph) {
        auto& in = v.in_edges();

        for (auto& iv : in) {
            tasks[graph[iv].name()].precede(tasks[v.name()]);
        }
    }
}

int GameClient::Run(tf::Executor& aExecutor)
{
    auto& window    = mRegistry.ctx().get<WatoWindow&>();
    auto& renderer  = mRegistry.ctx().get<Renderer&>();
    auto& netClient = mRegistry.ctx().get<ENetClient&>();
    auto  prevTime  = clock_type::now();

    aExecutor.silent_async([&]() { networkThread(); });

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

        if (mRegistry.ctx().contains<GameInstance>()) {
            auto& instance = mRegistry.ctx().get<GameInstance&>();

            for (const auto& system : mSystems) {
                system(mRegistry, frameTime.count());
            }

            AdvanceSimulation(mRegistry, frameTime.count());

            mUpdateTransformsSystem(mRegistry, instance.Accumulator / kTimeStep);
        }
        renderer.Render();
        input.PrevKeyboardState   = input.KeyboardState;
        input.PrevMouseState      = input.MouseState;
        input.MouseState.Scroll.y = 0;
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

    auto player = mRegistry.create();
    // TODO: ID should be something coming from outside (menu, DB, etc...)
    mRegistry.emplace<Player>(player, 0u);
    mRegistry.emplace<Name>(player, "stion");
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
    bgfx::updateTexture2D(
        texture,
        0,
        0,
        0,
        0,
        graph.Width(),
        graph.Height(),
        bgfx::copy(graph.GridLayout().data(), graph.Width() * graph.Height()));
}

void GameClient::OnGameInstanceCreated()
{
    spawnPlayerAndCamera();
    prepareGridPreview();
    mRegistry.ctx().get<Physics>().World()->setEventListener(&mPhysicsEventHandler);
}

void GameClient::consumeNetworkResponses()
{
    auto& netClient = mRegistry.ctx().get<ENetClient>();
    netClient.ConsumeNetworkResponses([&](NetworkResponse* aEvent) {
        std::visit(
            VariantVisitor{
                [&](const ConnectedResponse& aResp) {
                    BX_UNUSED(aResp);
                    WATO_INFO(mRegistry, "got connected response");
                    netClient.EnqueueRequest(new NetworkRequest{
                        .Type     = PacketType::NewGame,
                        .PlayerID = 0,
                        .Payload  = NewGameRequest{.PlayerAID = 0},
                    });
                },
                [&](const NewGameResponse& aResp) {
                    StartGameInstance(mRegistry, aResp.GameID, false);
                    WATO_INFO(mRegistry, "game {} created", aResp.GameID);
                },
                [&](SyncPayload& aResp) {
                    if (aResp.State.Snapshot.empty()) {
                        return;
                    }

                    BitInputArchive       inAr(aResp.State.Snapshot);
                    Registry              tmp;
                    entt::snapshot_loader loader{tmp};

                    WATO_DBG(
                        mRegistry,
                        "loading state snapshot {} of size {}",
                        aResp.State.Tick,
                        aResp.State.Snapshot.size());
                    loader.get<entt::entity>(inAr)
                        .get<Transform3D>(inAr)
                        .get<RigidBody>(inAr)
                        .get<Collider>(inAr);
                },
                [&](const std::monostate) {},
            },
            aEvent->Payload);
    });
}
