#pragma once

#include <bgfx/bgfx.h>

#include <array>

#include "renderer/framebuffer.hpp"
#include "renderer/render_types.hpp"

glm::vec4 EncodeEntity(uint32_t aEntity);

class Picker
{
   public:
    static constexpr uint16_t kTexDimension = 8;
    static constexpr uint32_t kBackground   = 0xffffffffu;
    static constexpr float    kFov          = 3.0f;

    Picker()
        : mFB(std::array{
              AttachmentDesc{
                  .TextureFmt = bgfx::TextureFormat::RGBA8,
                  .Width      = kTexDimension,
                  .Height     = kTexDimension,
                  .Flags = 0 | BGFX_TEXTURE_RT | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT
                           | BGFX_SAMPLER_MIP_POINT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP,
                  .Name = "picker.color"},
              AttachmentDesc{
                  .TextureFmt = bgfx::TextureFormat::D32F,
                  .Width      = kTexDimension,
                  .Height     = kTexDimension,
                  .Flags = 0 | BGFX_TEXTURE_RT | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT
                           | BGFX_SAMPLER_MIP_POINT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP,
                  .Name = "picker.depth"}}),
          mBlitTex(bgfx::createTexture2D(
              kTexDimension,
              kTexDimension,
              false,
              1,
              bgfx::TextureFormat::RGBA8,
              0 | BGFX_TEXTURE_BLIT_DST | BGFX_TEXTURE_READ_BACK | BGFX_SAMPLER_MIN_POINT
                  | BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_MIP_POINT | BGFX_SAMPLER_U_CLAMP
                  | BGFX_SAMPLER_V_CLAMP))
    {
    }

    ~Picker() = default;

    /**
     * @brief Reads from the picker texture if available, or request a read
     *
     * @param aCurrentFrame the frame we want to match against to check if data is ready
     * @return nullopt when buffer not ready or not requested, return the entity that was clicked
     * otherwise
     */
    std::optional<uint32_t> Read(uint32_t aCurrentFrame);

    void Request() { mRequested = true; }
    void Setup()
    {
        mFB.SetView(wato::kPickingPass);
        bgfx::setViewRect(wato::kPickingPass, 0, 0, Picker::kTexDimension, Picker::kTexDimension);
    }

   private:
    using blit_data_array = std::array<uint8_t, kTexDimension * kTexDimension * 4>;

    Framebuffer         mFB;
    bgfx::TextureHandle mBlitTex;
    blit_data_array     mBlitData{};
    uint32_t            mReadingFrame{};

    bool mRequested{};
};
