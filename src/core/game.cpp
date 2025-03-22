#include "core/game.hpp"

#include <bx/bx.h>

#include <memory>

#include "core/event_handler.hpp"
#include "systems/system.hpp"

void Game::Init()
{
    auto &engine = mRegistry.ctx().emplace<Engine>(std::make_unique<WatoWindow>(mWidth, mHeight),
        std::make_unique<Renderer>(),
        std::make_unique<Physics>());

    mRegistry.GetWindow().Init();
    mRegistry.GetRenderer().Init(mRegistry.GetWindow());

    auto &phy = engine.GetPhysics();
    phy.World = phy.Common.createPhysicsWorld();

    phy.World->setEventListener(new EventHandler(&mRegistry));

    // Create the default logger
    phy.Logger = phy.Common.createDefaultLogger();

    // Output the logs into the standard output
    phy.Logger->addStreamDestination(std::cout,
        static_cast<uint>(rp3d::Logger::Level::Error),
        rp3d::DefaultLogger::Format::Text);

    phy.InfoLogs    = false;
    phy.WarningLogs = false;
    phy.ErrorLogs   = true;

#if WATO_DEBUG
    phy.World->setIsDebugRenderingEnabled(true);
#endif

    mRegistry.LoadResources();
    mSystems.push_back(RenderImguiSystem::MakeDelegate(mRenderImguiSystem));
    mSystems.push_back(PlayerInputSystem::MakeDelegate(mPlayerInputSystem));
    mSystems.push_back(CameraSystem::MakeDelegate(mCameraSystem));
    mSystems.push_back(PhysicsSystem::MakeDelegate(mPhysicsSystem));
    mSystems.push_back(RenderSystem::MakeDelegate(mRenderSystem));

#if WATO_DEBUG
    mSystems.push_back(PhysicsDebugSystem::MakeDelegate(mPhysicsDbgSystem));
#endif
}

int Game::Run()
{
    auto &window   = mRegistry.GetWindow();
    auto &renderer = mRegistry.GetRenderer();

    // TODO: use std::chrono
    double prevTime = glfwGetTime();

    while (!window.ShouldClose()) {
        window.PollEvents();

        if (window.Resize()) {
            renderer.Resize(window);
        }

        auto t   = glfwGetTime();
        auto dt  = static_cast<float>(t - prevTime);
        prevTime = t;

        for (const auto &system : mSystems) {
            system(mRegistry, dt, window);
        }

        renderer.Render();
    }
    return 0;
}
