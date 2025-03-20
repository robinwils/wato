#include "core/game.hpp"

#include <bx/bx.h>

#include "bgfx/platform.h"
#include "bx/timer.h"
#include "core/event_handler.hpp"
#include "imgui_helper.h"

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

    mRegistry.Init(mWindow.get(), new EventHandler(&mRegistry, &mActionSystem));

    imguiCreate();

    mActionSystem.InitListeners(mWindow->GetInput());
}

int Game::Run()
{
    double  prevTime    = glfwGetTime();
    int64_t mTimeOffset = bx::getHPCounter();

    while (!mWindow->ShouldClose()) {
        mWindow->PollEvents();

        if (mWindow->Resize()) {
            bgfx::reset(mWindow->Width<uint32_t>(), mWindow->Height<uint32_t>(), BGFX_RESET_VSYNC);
            bgfx::setViewRect(CLEAR_VIEW, 0, 0, bgfx::BackbufferRatio::Equal);
            mActionSystem.UdpateWinSize(mWindow->Width<int>(), mWindow->Height<int>());
        }
        bgfx::touch(CLEAR_VIEW);
        // Use debug font to print information about this example.
        bgfx::dbgTextClear();

        renderImgui(mRegistry, *mWindow.get());

        auto t      = glfwGetTime();
        auto dt     = t - prevTime;
        prevTime    = t;
        double time = ((bx::getHPCounter() - mTimeOffset) / double(bx::getHPFrequency()));

        processInputs(mRegistry, dt);
        cameraSystem(mRegistry, mWindow->Width<float>(), mWindow->Height<float>());
        physicsSystem(mRegistry, dt);

        // This dummy draw call is here to make sure that view 0 is cleared
        // if no other draw calls are submitted to view 0.
        bgfx::touch(0);

        renderSceneObjects(mRegistry, time);
#if WATO_DEBUG
        physicsDebugRenderSystem(mRegistry);
#endif

        // Advance to next frame. Process submitted rendering primitives.
        bgfx::frame();
    }
    return 0;
}
