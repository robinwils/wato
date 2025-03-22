#include "core/game.hpp"

#include <bx/bx.h>

#include "core/event_handler.hpp"
#include "systems/system.hpp"

void Game::Init()
{
    mWindow.Init();
    mRenderer.Init(mWindow);
    mRegistry.Init(mWindow, new EventHandler(&mRegistry));

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
    // TODO: use std::chrono
    double prevTime = glfwGetTime();

    while (!mWindow.ShouldClose()) {
        mWindow.PollEvents();

        if (mWindow.Resize()) {
            mRenderer.Resize(mWindow);
        }

        auto t   = glfwGetTime();
        auto dt  = static_cast<float>(t - prevTime);
        prevTime = t;

        for (const auto& system : mSystems) {
            system(mRegistry, dt, mWindow);
        }

        mRenderer.Render();
    }
    return 0;
}
