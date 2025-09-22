#include "renderer/renderer.hpp"

#include <bgfx/platform.h>
#include <bx/bx.h>

#include "core/window.hpp"
#include "imgui_helper.h"

void Renderer::Init(WatoWindow& aWin)
{
    if (!aWin.IsInitialized()) {
        throw std::runtime_error("window not initialized");
    }

#ifdef BGFX_CONFIG_MULTITHREADED
    // Call bgfx::renderFrame before bgfx::init to signal to bgfx not to create a render thread.
    // Most graphics APIs must be used on the same thread that created the window.
    // bgfx disables multithreaded config when configured with glfw (see scripts/bgfx.lua)
    bgfx::renderFrame();
#endif

    mInitParams.platformData.ndt = aWin.GetNativeDisplay();
    mInitParams.platformData.nwh = aWin.GetNativeWindow();
    if (aWin.UseWayland()) {
        mInitParams.platformData.type = bgfx::NativeWindowHandleType::Wayland;
    }

    if (mInitParams.platformData.ndt == nullptr && mInitParams.platformData.nwh == nullptr) {
        throw std::runtime_error("cannot get native window and display");
    }

    mInitParams.type = mRenderer;

#if WATO_DEBUG
    mInitParams.debug = true;
#endif

    mInitParams.resolution.width  = aWin.Width<uint32_t>();
    mInitParams.resolution.height = aWin.Height<uint32_t>();
    mInitParams.resolution.reset  = BGFX_RESET_VSYNC;

    if (!bgfx::init(mInitParams)) {
        throw std::runtime_error("cannot init graphics");
    }

#if WATO_DEBUG
    // Enable stats or debug text.
    bgfx::setDebug(BGFX_DEBUG_TEXT);
#endif

    // Set view 0 clear state.
    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x0090cfff, 1.0f, 0);

    imguiCreate();
    mIsInit = true;
}

bgfx::RendererType::Enum Renderer::detectRenderer(const std::string& aRenderer) const
{
    if (aRenderer == "vulkan" || aRenderer == "vk") {
        return bgfx::RendererType::Vulkan;
    } else if (aRenderer == "metal" || aRenderer == "mtl") {
        return bgfx::RendererType::Metal;
    } else if (aRenderer == "opengl" || aRenderer == "ogl") {
        return bgfx::RendererType::OpenGL;
    } else if (aRenderer == "opengles" || aRenderer == "ogles") {
        return bgfx::RendererType::OpenGLES;
    } else {
#if BX_PLATFORM_OSX
        return bgfx::RendererType::Metal;
#elif BX_PLATFORM_WINDOWS
        return bgfx::RendererType::Direct3D12;
#else
        return bgfx::RendererType::Vulkan;
#endif
    }
}

void Renderer::Resize(WatoWindow& aWin)
{
    bgfx::reset(aWin.Width<uint32_t>(), aWin.Height<uint32_t>(), BGFX_RESET_VSYNC);
    bgfx::setViewRect(kClearView, 0, 0, bgfx::BackbufferRatio::Equal);
}

void Renderer::Clear()
{
    bgfx::touch(kClearView);
    bgfx::dbgTextClear();
}

void Renderer::Render()
{
    // Advance to next frame. Process submitted rendering primitives.
    bgfx::frame();
}
