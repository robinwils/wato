#include "renderer/renderer.hpp"

#include <bx/bx.h>

#include "core/window.hpp"
#include "imgui_helper.h"

void BgfxRenderer::Init(WatoWindow& aWin)
{
    if (!aWin.IsInitialized()) {
        throw std::runtime_error("window not initialized");
    }

    mInitParams.swapChain.ndt = aWin.GetNativeDisplay();
    mInitParams.swapChain.nwh = aWin.GetNativeWindow();
    if (aWin.UseWayland()) {
        mInitParams.platformData.type = bgfx::NativeWindowHandleType::Wayland;
    }

    if (mInitParams.swapChain.ndt == nullptr && mInitParams.swapChain.nwh == nullptr) {
        throw std::runtime_error("cannot get native window and display");
    }

    mInitParams.type = mRenderer;

#if WATO_DEBUG
    mInitParams.debug = true;
#endif

    mInitParams.swapChain.width  = aWin.Width<uint32_t>();
    mInitParams.swapChain.height = aWin.Height<uint32_t>();
    mInitParams.reset  = BGFX_RESET_VSYNC;

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

bgfx::RendererType::Enum BgfxRenderer::detectRenderer(const std::string& aRenderer) const
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

void BgfxRenderer::Resize(WatoWindow& aWin)
{
    bgfx::SwapChain swapChain;
    constexpr uint32_t kSwapChainFlags = 0
			| BGFX_SWAP_CHAIN_FULLSCREEN_MASK
			| BGFX_SWAP_CHAIN_MSAA_MASK
			| BGFX_SWAP_CHAIN_SRGB_BACKBUFFER
			| BGFX_SWAP_CHAIN_HDR10
			| BGFX_SWAP_CHAIN_HIDPI
			| BGFX_SWAP_CHAIN_TRANSPARENT_BACKBUFFER
			;
    swapChain.width = aWin.Width<uint32_t>();
    swapChain.height = aWin.Height<uint32_t>();

    // TODO: configurable VSYNC/MSAA etc...
    swapChain.flags  = BGFX_RESET_VSYNC &  kSwapChainFlags;

    bgfx::reset(BGFX_RESET_VSYNC & ~kSwapChainFlags, &swapChain);
    bgfx::reset(kSwapChainFlags, &swapChain);
    bgfx::setViewRect(kClearView, 0, 0, bgfx::BackbufferRatio::Equal);
}

void BgfxRenderer::Clear()
{
    bgfx::touch(kClearView);
    bgfx::dbgTextClear();
}

void BgfxRenderer::Render()
{
    // Advance to next frame. Process submitted rendering primitives.
    bgfx::frame();
}

void BgfxRenderer::SubmitDebugGeometry(
    const void*               aData,
    uint32_t                  aNumVerts,
    uint64_t                  aState,
    bgfx::ProgramHandle       aProgram,
    const bgfx::VertexLayout& aLayout)
{
    if (aNumVerts == 0) {
        return;
    }

    if (aNumVerts != bgfx::getAvailTransientVertexBuffer(aNumVerts, aLayout)) {
        return;
    }

    bgfx::TransientVertexBuffer vb{};
    bgfx::allocTransientVertexBuffer(&vb, aNumVerts, aLayout);
    bx::memCopy(vb.data, aData, aNumVerts * aLayout.getStride());

    bgfx::setState(aState);
    bgfx::setVertexBuffer(0, &vb);
    bgfx::submit(0, aProgram, bgfx::ViewMode::Default);
}
