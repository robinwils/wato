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

    engine.GetWindow().Init();
    engine.GetRenderer().Init(mRegistry.GetWindow());
    engine.GetPhysics().Init(&mRegistry);

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
