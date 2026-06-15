#pragma once

#include <bgfx/bgfx.h>
#include <bx/bx.h>

#include <vector>

#include "core/types.hpp"

struct AttachmentDesc {
    bgfx::TextureFormat::Enum TextureFmt;

    uint16_t    Width, Height;
    uint64_t    Flags = BGFX_TEXTURE_RT;
    const char* Name;
};

class Framebuffer
{
   public:
    Framebuffer(std::span<const AttachmentDesc> aDescs)
        : mHandles(makeAttachments(aDescs)),
          mFBHandle(bgfx::createFrameBuffer(SafeU8(mHandles.size()), mHandles.data(), true))
    {
    }

    ~Framebuffer() { bgfx::destroy(mFBHandle); }

    void SetView(bgfx::ViewId aID) { bgfx::setViewFrameBuffer(aID, mFBHandle); }

    [[nodiscard]] bgfx::FrameBufferHandle Handle() const { return mFBHandle; }

   private:
    static std::vector<bgfx::TextureHandle> makeAttachments(std::span<const AttachmentDesc> aDescs)
    {
        std::vector<bgfx::TextureHandle> handles;
        handles.reserve(aDescs.size());

        // TODO: could use bgfx::Attachment
        for (auto desc : aDescs) {
            bgfx::TextureHandle handle = bgfx::createTexture2D(
                desc.Width,
                desc.Height,
                false,
                1,
                desc.TextureFmt,
                desc.Flags);
            bgfx::setName(handle, desc.Name);
            handles.push_back(handle);
        }

        return handles;
    }

    std::vector<bgfx::TextureHandle> mHandles;
    bgfx::FrameBufferHandle          mFBHandle;
};
