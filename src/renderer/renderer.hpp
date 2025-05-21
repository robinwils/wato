#pragma once

#include <glm/gtc/type_ptr.hpp>

#include "bgfx/bgfx.h"
#include "core/window.hpp"

class Renderer
{
   public:
    Renderer() {}
    Renderer(Renderer&&)                 = default;
    Renderer(const Renderer&)            = default;
    Renderer& operator=(Renderer&&)      = delete;
    Renderer& operator=(const Renderer&) = delete;
    ~Renderer()                          = default;

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
    const bgfx::ViewId CLEAR_VIEW = 0;
    bgfx::Init         mInitParams;
    bool               mIsInit;
};
