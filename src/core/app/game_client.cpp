#include "core/app/game_client.hpp"

#include <bx/bx.h>
#include <fmt/base.h>

#include <chrono>
#include <taskflow/taskflow.hpp>
#include <thread>

#include "components/game.hpp"
#include "components/imgui.hpp"
#include "components/tile.hpp"
#include "core/net/enet_client.hpp"
#include "core/net/net.hpp"
#include "core/sys/log.hpp"
#include "core/window.hpp"
#include "registry/game_registry.hpp"
#include "registry/registry.hpp"
#include "renderer/renderer.hpp"
#include "systems/render.hpp"
#include "systems/system.hpp"

using namespace std::literals::chrono_literals;

void GameClient::Init()
{
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
    mSystems.push_back(RealTimeActionSystem::MakeDelegate(mActionSystem));
    mSystems.push_back(CameraSystem::MakeDelegate(mCameraSystem));
    mSystems.push_back(RenderSystem::MakeDelegate(mRenderSystem));

#if WATO_DEBUG
    mSystems.push_back(PhysicsDebugSystem::MakeDelegate(mPhysicsDbgSystem));
#endif

    mSystemsFT.push_back(DeterministicActionSystem::MakeDelegate(mFTActionSystem));
    mSystemsFT.push_back(PhysicsSystem::MakeDelegate(mPhysicsSystem));
    mSystemsFT.push_back(NetworkSyncSystem::MakeDelegate(mNetworkSyncSystem));

    auto graph = organizerFixedTime.graph();
    INFO("graph size: %ld", graph.size());
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

int GameClient::Run()
{
    tf::Executor executor;

    auto&                       window    = mRegistry.ctx().get<WatoWindow&>();
    auto&                       renderer  = mRegistry.ctx().get<Renderer&>();
    auto&                       netClient = mRegistry.ctx().get<ENetClient&>();
    auto                        prevTime  = clock_type::now();
    std::optional<std::jthread> netPollThread;

    if (mOptions.Multiplayer()) {
        netPollThread.emplace(&GameClient::networkThread, this);

        if (!netClient.Connect()) {
            throw std::runtime_error("No available peers for initiating an ENet connection.");
        }
    } else {
        StartGameInstance(mRegistry, 0);
        spawnPlayerAndCamera();
    }

    while (!window.ShouldClose()) {
        window.PollEvents();

        if (window.Resize()) {
            renderer.Resize(window);
        }

        Input&                       input     = window.GetInput();
        auto                         now       = clock_type::now();
        std::chrono::duration<float> frameTime = (now - prevTime);

        prevTime = now;

        if (mOptions.Multiplayer()) {
            consumeNetworkResponses();
        }

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

    if (mOptions.Multiplayer()) {
        netClient.Disconnect();
        mDiscTimerStart.emplace(clock_type::now());
    }
    return 0;
}

void GameClient::networkThread()
{
    auto& netClient = mRegistry.ctx().get<ENetClient&>();
    while (netClient.Running()) {
        if (mDiscTimerStart.has_value()) {
            if (clock_type::now() - mDiscTimerStart.value() > 3s) {
                netClient.ForceDisconnect();
            }
        }

        netClient.ConsumeNetworkRequests();
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
        glm::vec3(0.0f, 2.0f, 1.5f),
        glm::identity<glm::quat>(),
        glm::vec3(1.0f));
    mRegistry.emplace<ImguiDrawable>(camera, "Camera");

    auto player = mRegistry.create();
    // TODO: ID should be something coming from outside (menu, DB, etc...)
    mRegistry.emplace<Player>(player, 0u, "stion", camera);
}

void GameClient::consumeNetworkResponses()
{
    auto& netClient = mRegistry.ctx().get<ENetClient>();
    while (const NetworkEvent<NetworkResponsePayload>* ev = netClient.ResponseQueue().pop()) {
        std::visit(
            VariantVisitor{
                [&](const ConnectedResponse& aResp) {
                    netClient.EnqueueSend(new NetworkEvent<NetworkRequestPayload>{
                        .Type     = PacketType::NewGame,
                        .PlayerID = 0,
                        .Payload  = NewGameRequest{.PlayerAID = 0},
                    });
                },
                [&](const NewGameResponse& aResp) {
                    StartGameInstance(mRegistry, aResp.GameID);
                    spawnPlayerAndCamera();
                    fmt::println("game {} created", aResp.GameID);
                },
            },
            ev->Payload);
        delete ev;
    }
}
