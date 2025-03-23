#include "core/app/game.hpp"

#include <bx/bx.h>

#include <memory>

#include "core/physics.hpp"
#include "core/window.hpp"
#include "registry/game_registry.hpp"
#include "renderer/renderer.hpp"
#include "systems/system.hpp"

void Game::Init()
{
    auto &window   = mRegistry.ctx().emplace<WatoWindow>(mWidth, mHeight);
    auto &renderer = mRegistry.ctx().emplace<Renderer>();
    auto &physics  = mRegistry.ctx().emplace<Physics>();

    window.Init();
    renderer.Init(window);
    physics.Init();

    // TODO: leak ?
    physics.World()->setEventListener(new EventHandler(&mRegistry));

    LoadResources(mRegistry);
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
    auto &window   = mRegistry.ctx().get<WatoWindow &>();
    auto &renderer = mRegistry.ctx().get<Renderer &>();

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
            system(mRegistry, dt.count());
        }

        renderer.Render();
    }
    return 0;
}
