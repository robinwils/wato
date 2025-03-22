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
    phy.InfoLogs    = false;
    phy.WarningLogs = false;
    phy.ErrorLogs   = true;
    phy.Logger      = phy.Common.createDefaultLogger();
    uint logLevel   = 0;
    if (phy.InfoLogs) {
        logLevel |= static_cast<uint>(rp3d::Logger::Level::Information);
    }
    if (phy.WarningLogs) {
        logLevel |= static_cast<uint>(rp3d::Logger::Level::Warning);
    }
    if (phy.ErrorLogs) {
        logLevel |= static_cast<uint>(rp3d::Logger::Level::Error);
    }

    // Output the logs into an HTML file
    phy.Logger->addFileDestination("rp3d_log.html", logLevel, rp3d::DefaultLogger::Format::HTML);

    // Output the logs into the standard output
    phy.Logger->addStreamDestination(std::cout, logLevel, rp3d::DefaultLogger::Format::Text);
    phy.Common.setLogger(phy.Logger);

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

    using clock   = std::chrono::high_resolution_clock;
    auto prevTime = clock::now();

    while (!window.ShouldClose()) {
        window.PollEvents();

        if (window.Resize()) {
            renderer.Resize(window);
        }

        auto                         t  = clock::now();
        std::chrono::duration<float> dt = (t - prevTime);

        prevTime = t;

        for (const auto &system : mSystems) {
            system(mRegistry, dt.count(), window);
        }

        renderer.Render();
    }
    return 0;
}
