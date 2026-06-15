#include "renderer/renderer.hpp"

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/bx.h>

#include <memory>

#include "core/window.hpp"
#include "imgui_helper.h"

void BgfxRenderer::Init(WatoWindow& aWin)
{
    if (!aWin.IsInitialized()) {
        throw std::runtime_error("window not initialized");
    }

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

    bgfx::setViewClear(wato::kRenderPass, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x0090cfff, 1.0f, 0);
    bgfx::setViewClear(
        wato::kPickingPass,
        BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
        Picker::kBackground,
        1.0f,
        0);

    imguiCreate();
    mPicker = std::make_unique<Picker>();
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
    bgfx::reset(aWin.Width<uint32_t>(), aWin.Height<uint32_t>(), BGFX_RESET_VSYNC);
    bgfx::setViewRect(wato::kRenderPass, 0, 0, bgfx::BackbufferRatio::Equal);
}

void BgfxRenderer::Clear()
{
    bgfx::touch(wato::kRenderPass);
    bgfx::dbgTextClear();
}

void BgfxRenderer::SetupPickingPass() { mPicker->Setup(); }

void BgfxRenderer::Render()
{
    // Advance to next frame. Process submitted rendering primitives.
    mCurrentFrame = bgfx::frame();
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
    bgfx::submit(wato::kRenderPass, aProgram, bgfx::ViewMode::Default);
}
