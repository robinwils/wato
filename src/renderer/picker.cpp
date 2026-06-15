#pragma once

#include "renderer/picker.hpp"

#include <map>

#include "renderer/render_types.hpp"

glm::vec4 EncodeEntity(uint32_t aEntity)
{
    return {
        float((aEntity >> 0) & 0xFF) / 255.0f,
        float((aEntity >> 8) & 0xFF) / 255.0f,
        float((aEntity >> 16) & 0xFF) / 255.0f,
        float((aEntity >> 24) & 0xFF) / 255.0f,
    };
}

namespace
{
uint32_t DecodeEntity(std::span<uint8_t, 4> aData)
{
    return uint32_t(aData[3]) << 24 | uint32_t(aData[2]) << 16 | uint32_t(aData[1]) << 8
           | uint32_t(aData[0]);
}
};  // namespace

std::optional<uint32_t> Picker::Read(uint32_t aCurrentFrame)
{
    if (mReadingFrame == aCurrentFrame) {
        mReadingFrame = 0;

        uint32_t                     maxAmount    = 0;
        uint32_t                     chosenEntity = kBackground;
        std::map<uint32_t, uint32_t> entityCount;

        for (size_t idx = 0; idx < mBlitData.size(); idx += 4) {
            uint32_t entity = DecodeEntity(std::span<uint8_t, 4>{mBlitData.data() + idx, 4});

            if (kBackground == entity) {
                continue;
            }

            uint32_t amount = ++entityCount[entity];
            if (amount > maxAmount) {
                maxAmount    = amount;
                chosenEntity = entity;
            }
        }

        // can return optional<0xffffffff> but that means we clicked on nothing
        return chosenEntity;
    }

    if (mRequested && mReadingFrame == 0u) {
        bgfx::TextureHandle pickingTex = bgfx::getTexture(mFB.Handle(), 0);
        bgfx::blit(wato::kPickingPass, mBlitTex, 0, 0, pickingTex);
        mReadingFrame = bgfx::readTexture(mBlitTex, mBlitData.data());
        mRequested    = false;
    }

    return std::nullopt;
}
