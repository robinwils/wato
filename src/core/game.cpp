#include "core/game.hpp"

#include <bx/bx.h>

#include "bgfx/platform.h"
#include "bx/timer.h"
#include "core/event_handler.hpp"
#include "imgui_helper.h"
#include "systems/system.hpp"

void Game::Init()
{
    mWindow->Init();
    // Call bgfx::renderFrame before bgfx::init to signal to bgfx not to create a render thread.
    // Most graphics APIs must be used on the same thread that created the window.
    bgfx::renderFrame();
    mInitParams.platformData.ndt = mWindow->GetNativeDisplay();
    mInitParams.platformData.nwh = mWindow->GetNativeWindow();

    if (mInitParams.platformData.ndt == nullptr && mInitParams.platformData.nwh == nullptr) {
        throw std::runtime_error("cannot get native window and display");
    }

#if BX_PLATFORM_OSX
    mInitParams.type = bgfx::RendererType::Metal;
#else
    mInitParams.type = bgfx::RendererType::Vulkan;
#endif

    mInitParams.resolution.width  = mWindow->Width<uint32_t>();
    mInitParams.resolution.height = mWindow->Height<uint32_t>();
    mInitParams.resolution.reset  = BGFX_RESET_VSYNC;

    if (!bgfx::init(mInitParams)) {
        throw std::runtime_error("cannot init graphics");
    }

    // Enable stats or debug text.
    bgfx::setDebug(BGFX_DEBUG_TEXT | BGFX_DEBUG_PROFILER);

    // Set view 0 clear state.
    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x0090cfff, 1.0f, 0);

    mRegistry.Init(mWindow.get(), new EventHandler(&mRegistry));

    imguiCreate();

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
    double prevTime = glfwGetTime();

    while (!mWindow->ShouldClose()) {
        mWindow->PollEvents();

        if (mWindow->Resize()) {
            bgfx::reset(mWindow->Width<uint32_t>(), mWindow->Height<uint32_t>(), BGFX_RESET_VSYNC);
            bgfx::setViewRect(CLEAR_VIEW, 0, 0, bgfx::BackbufferRatio::Equal);
        }
        bgfx::touch(CLEAR_VIEW);
        // Use debug font to print information about this example.
        bgfx::dbgTextClear();

        auto t   = glfwGetTime();
        auto dt  = static_cast<float>(t - prevTime);
        prevTime = t;

        for (const auto& system : mSystems) {
            system(mRegistry, dt, *mWindow);
        }

        // Advance to next frame. Process submitted rendering primitives.
        bgfx::frame();
    }
    return 0;
}
