#pragma once

#include <bgfx/bgfx.h>

#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <renderer/material.hpp>
#include <string>
#include <utility>
#include <vector>

#include "glm/fwd.hpp"

struct PositionNormalUvVertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 Uv;

    static bgfx::VertexLayout GetVertexLayout()
    {
        bgfx::VertexLayout vertexLayout;
        vertexLayout.begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
            .end();
        return vertexLayout;
    }
};

template <typename VL>
class Primitive
{
   public:
    typedef VL layout_type;

    Primitive(Material* aMaterial) : mMaterial(aMaterial), mIsInitialized(false) {}
    Primitive(Material*          aMaterial,
        std::vector<layout_type> aVertices,
        std::vector<uint16_t>    aIndices)
        : mVertices(aVertices), mIndices(aIndices), mMaterial(aMaterial)
    {
    }
    Primitive(const Primitive& aOther) noexcept
        : mVertices(aOther.mVertices),
          mIndices(aOther.mIndices),
          mMaterial(aOther.mMaterial),
          mIsInitialized(aOther.mIsInitialized),
          mVertexBufferHandle(aOther.mVertexBufferHandle),
          mIndexBufferHandle(aOther.mIndexBufferHandle)
    {
    }
    Primitive(Primitive&& aOther) noexcept
        : mVertices(std::move(aOther.mVertices)),
          mIndices(std::move(aOther.mIndices)),
          mMaterial(std::move(aOther.mMaterial)),
          mVertexBufferHandle(aOther.mVertexBufferHandle),
          mIndexBufferHandle(aOther.mIndexBufferHandle),

          mIsInitialized(aOther.mIsInitialized)
    {
        aOther.mVertexBufferHandle.idx = bgfx::kInvalidHandle;
        aOther.mIndexBufferHandle.idx  = bgfx::kInvalidHandle;
        aOther.mIsInitialized          = false;
    }

    Primitive& operator=(Primitive& aOther)
    {
        mVertexBufferHandle = aOther.mVertexBufferHandle;
        mIndexBufferHandle  = aOther.mIndexBufferHandle;
        mVertices           = std::move(aOther.mVertices);
        mIndices            = std::move(aOther.mIndices);
        return *this;
    }
    Primitive& operator=(Primitive&& aOther)
    {
        mVertexBufferHandle = aOther.mVertexBufferHandle;
        mIndexBufferHandle  = aOther.mIndexBufferHandle;
        mVertices           = std::move(aOther.mVertices);
        mIndices            = std::move(aOther.mIndices);
        return *this;
    }
    virtual ~Primitive() { destroyPrimitive(); }

    virtual void Submit(uint8_t aDiscardStates = BGFX_DISCARD_ALL) const
    {
        mMaterial->Submit();
        assert(mIsInitialized);

        bgfx::setVertexBuffer(0, mVertexBufferHandle);
        bgfx::setIndexBuffer(mIndexBufferHandle);

        bgfx::submit(0, mMaterial->Program(), bgfx::ViewMode::Default, aDiscardStates);
    }

    virtual void InitializePrimitive()
    {
        assert(!mVertices.empty());
        assert(!mIndices.empty());

        const bgfx::VertexLayout vertexLayout = layout_type::GetVertexLayout();

        mVertexBufferHandle = bgfx::createVertexBuffer(
            bgfx::makeRef(mVertices.data(), sizeof(layout_type) * mVertices.size()),
            vertexLayout);
        mIndexBufferHandle = bgfx::createIndexBuffer(
            bgfx::makeRef(mIndices.data(), sizeof(uint16_t) * mIndices.size()));

        mIsInitialized = true;
    }

   protected:
    std::vector<layout_type> mVertices;
    std::vector<uint16_t>    mIndices;
    Material*                mMaterial;

    bool mIsInitialized;

    bgfx::VertexBufferHandle mVertexBufferHandle;
    bgfx::IndexBufferHandle  mIndexBufferHandle;

   private:
    virtual void destroyPrimitive()
    {
        if (mIsInitialized) {
            DBG("destroying vertex buffer %d and index buffer %d",
                mVertexBufferHandle.idx,
                mIndexBufferHandle.idx);
            bgfx::destroy(mVertexBufferHandle);
            bgfx::destroy(mIndexBufferHandle);
        }
    }
};
