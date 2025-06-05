#include "renderer/renderer.hpp"

#include <bx/bx.h>

#include "bgfx/platform.h"
#include "core/window.hpp"
#include "imgui_helper.h"

void Renderer::Init(WatoWindow& aWin)
{
    if (!aWin.IsInitialized()) {
        throw std::runtime_error("window not initialized");
    }

    // Call bgfx::renderFrame before bgfx::init to signal to bgfx not to create a render thread.
    // Most graphics APIs must be used on the same thread that created the window.
    bgfx::renderFrame();

    mInitParams.platformData.ndt = aWin.GetNativeDisplay();
    mInitParams.platformData.nwh = aWin.GetNativeWindow();

    if (mInitParams.platformData.ndt == nullptr && mInitParams.platformData.nwh == nullptr) {
        throw std::runtime_error("cannot get native window and display");
    }

#if BX_PLATFORM_OSX
    mInitParams.type = bgfx::RendererType::Metal;
#else
    mInitParams.type = bgfx::RendererType::Vulkan;
#endif

    mInitParams.resolution.width  = aWin.Width<uint32_t>();
    mInitParams.resolution.height = aWin.Height<uint32_t>();
    mInitParams.resolution.reset  = BGFX_RESET_VSYNC;

    if (!bgfx::init(mInitParams)) {
        throw std::runtime_error("cannot init graphics");
    }

    // Enable stats or debug text.
    bgfx::setDebug(BGFX_DEBUG_TEXT | BGFX_DEBUG_PROFILER);

    // Set view 0 clear state.
    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x0090cfff, 1.0f, 0);

    imguiCreate();
    mIsInit = true;
}

void Renderer::Resize(WatoWindow& aWin)
{
    bgfx::reset(aWin.Width<uint32_t>(), aWin.Height<uint32_t>(), BGFX_RESET_VSYNC);
    bgfx::setViewRect(kClearView, 0, 0, bgfx::BackbufferRatio::Equal);
}

void Renderer::Clear()
{
    bgfx::touch(kClearView);
    // Use debug font to print information about this example.
    bgfx::dbgTextClear();
}

void Renderer::Render()
{
    // Advance to next frame. Process submitted rendering primitives.
    bgfx::frame();
}
