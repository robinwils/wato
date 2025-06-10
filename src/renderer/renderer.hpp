#pragma once

#include <bgfx/bgfx.h>

#include <glm/gtc/type_ptr.hpp>

#include "core/window.hpp"

class Renderer
{
   public:
    Renderer(std::string aRenderer) : mIsInit(false), mRenderer(detectRenderer(aRenderer)) {}

    void Init(WatoWindow& aWin);
    void Resize(WatoWindow& aWin);
    void Clear();
    void Render();

    void SetTransform(glm::mat4 aModel) { bgfx::setTransform(glm::value_ptr(aModel)); }
    void SetUniform(bgfx::UniformHandle aHandle, glm::vec4 aUniform)
    {
        bgfx::setUniform(aHandle, glm::value_ptr(aUniform));
    }

    [[nodiscard]] bool IsInitialized() const noexcept { return mIsInit; }

   private:
    bgfx::RendererType::Enum detectRenderer(const std::string& aRenderer) const;

    static constexpr bgfx::ViewId kClearView = 0;

    bgfx::Init               mInitParams;
    bool                     mIsInit;
    bgfx::RendererType::Enum mRenderer;
};
