#pragma once

#include <cstdint>
#include <glm/ext/matrix_float4x4.hpp>
#include <span>
#include <string>

#include "render_types.hpp"

class WatoWindow;

class HeadlessRenderer
{
   public:
    HeadlessRenderer(std::string) {}

    void Init(WatoWindow&) { mIsInit = true; }
    void Resize(WatoWindow&) {}
    void Clear() {}
    void Render() {}

    void Touch(wato::ViewId) {}
    void SetViewTransform(wato::ViewId, const glm::mat4&, const glm::mat4&) {}
    void SetViewRect(wato::ViewId, uint16_t, uint16_t, uint16_t, uint16_t) {}
    void SetTransform(glm::mat4) {}

    template <typename T>
    void SetUniform(auto, const T&, uint16_t = 1)
    {
    }

    void UpdateTexture2D(auto, uint16_t, uint8_t, uint16_t, uint16_t, uint16_t, uint16_t, std::span<const uint8_t>)
    {
    }

    void SubmitDebugGeometry(const void*, uint32_t, uint64_t, auto, auto) {}

    [[nodiscard]] bool IsInitialized() const noexcept { return mIsInit; }

   private:
    bool mIsInit = false;
};
