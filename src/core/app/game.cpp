#include "core/app/game.hpp"

#include <bx/bx.h>

#include <taskflow/taskflow.hpp>
#include <thread>

#include "core/net/enet_client.hpp"
#include "core/physics.hpp"
#include "core/sys/log.hpp"
#include "core/window.hpp"
#include "registry/game_registry.hpp"
#include "renderer/renderer.hpp"
#include "systems/render.hpp"
#include "systems/system.hpp"

void Game::Init()
{
    auto& window    = mRegistry.ctx().get<WatoWindow>();
    auto& renderer  = mRegistry.ctx().get<Renderer>();
    auto& physics   = mRegistry.ctx().get<Physics>();
    auto& netClient = mRegistry.ctx().get<ENetClient>();

    window.Init();
    renderer.Init(window);
    physics.Init();
    netClient.Init();

    // TODO: leak ?
    physics.World()->setEventListener(new EventHandler(&mRegistry));

    entt::organizer organizerFixedTime;
    LoadResources(mRegistry);
    mSystems.push_back(RenderImguiSystem::MakeDelegate(mRenderImguiSystem));
    mSystems.push_back(PlayerInputSystem::MakeDelegate(mPlayerInputSystem));
    mSystems.push_back(CameraSystem::MakeDelegate(mCameraSystem));
    mSystems.push_back(PhysicsSystem::MakeDelegate(mPhysicsSystem));
    mSystems.push_back(RenderSystem::MakeDelegate(mRenderSystem));

#if WATO_DEBUG
    mSystems.push_back(PhysicsDebugSystem::MakeDelegate(mPhysicsDbgSystem));
#endif
    auto graph = organizerFixedTime.graph();
    INFO("graph size: %ld", graph.size());
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

int Game::Run()
{
    tf::Executor    executor;
    constexpr float timeStep = 1.0f / 60.0f;

    auto& window    = mRegistry.ctx().get<WatoWindow&>();
    auto& renderer  = mRegistry.ctx().get<Renderer&>();
    auto& netClient = mRegistry.ctx().get<ENetClient&>();
    auto& opts      = mRegistry.ctx().get<Options&>();

    float accumulator                    = 0.0f;
    using clock                          = std::chrono::steady_clock;
    auto                        prevTime = clock::now();
    std::optional<std::jthread> netPollThread;

    if (opts.Multiplayer()) {
        netPollThread.emplace([&]() {
            while (netClient.Running()) {
                netClient.ConsumeEvents(nullptr);
                netClient.Poll();
            }
        });

        if (!netClient.Connect()) {
            throw std::runtime_error("No available peers for initiating an ENet connection.");
        }
    }

    while (!window.ShouldClose()) {
        window.PollEvents();

        if (window.Resize()) {
            renderer.Resize(window);
        }

        auto                         t   = clock::now();
        std::chrono::duration<float> dt  = (t - prevTime);
        accumulator                     += dt.count();

        prevTime = t;

        // This dummy draw call is here to make sure that view 0 is cleared
        // if no other draw calls are submitted to view 0.
        bgfx::touch(0);

        // While there is enough accumulated time to take
        // one or several physics steps
        while (accumulator >= timeStep) {
            // Decrease the accumulated time
            accumulator -= timeStep;
        }

        for (const auto& system : mSystems) {
            system(mRegistry, dt.count());
        }

        renderer.Render();
    }

    if (opts.Multiplayer()) {
        netClient.Disconnect();
    }
    return 0;
}
