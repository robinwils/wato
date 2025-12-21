#pragma once

#include <bgfx/bgfx.h>

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <span>

#include "core/window.hpp"
#include "render_types.hpp"

class BgfxRenderer
{
   public:
    BgfxRenderer(std::string aRenderer) : mIsInit(false), mRenderer(detectRenderer(aRenderer)) {}

    void Init(WatoWindow& aWin);
    void Resize(WatoWindow& aWin);
    void Clear();
    void Render();

    void Touch(wato::ViewId aViewId) { bgfx::touch(aViewId); }

    void SetViewTransform(wato::ViewId aViewId, const glm::mat4& aView, const glm::mat4& aProj)
    {
        bgfx::setViewTransform(aViewId, glm::value_ptr(aView), glm::value_ptr(aProj));
    }

    void
    SetViewRect(wato::ViewId aViewId, uint16_t aX, uint16_t aY, uint16_t aWidth, uint16_t aHeight)
    {
        bgfx::setViewRect(aViewId, aX, aY, aWidth, aHeight);
    }

    void SetTransform(glm::mat4 aModel) { bgfx::setTransform(glm::value_ptr(aModel)); }

    void SetUniform(bgfx::UniformHandle aHandle, const auto& aUniform, uint16_t aNum = 1)
    {
        bgfx::setUniform(aHandle, glm::value_ptr(aUniform), aNum);
    }

    void UpdateTexture2D(
        bgfx::TextureHandle      aHandle,
        uint16_t                 aLayer,
        uint8_t                  aMip,
        uint16_t                 aX,
        uint16_t                 aY,
        uint16_t                 aWidth,
        uint16_t                 aHeight,
        std::span<const uint8_t> aData)
    {
        bgfx::updateTexture2D(
            aHandle,
            aLayer,
            aMip,
            aX,
            aY,
            aWidth,
            aHeight,
            bgfx::copy(aData.data(), static_cast<uint32_t>(aData.size())));
    }

    void SubmitDebugGeometry(
        const void*               aData,
        uint32_t                  aNumVerts,
        uint64_t                  aState,
        bgfx::ProgramHandle       aProgram,
        const bgfx::VertexLayout& aLayout);

    [[nodiscard]] bool IsInitialized() const noexcept { return mIsInit; }

   private:
    bgfx::RendererType::Enum detectRenderer(const std::string& aRenderer) const;

    static constexpr bgfx::ViewId kClearView = 0;

    bgfx::Init               mInitParams;
    bool                     mIsInit;
    bgfx::RendererType::Enum mRenderer;
};

// Type alias for backwards compatibility
using Renderer = BgfxRenderer;
