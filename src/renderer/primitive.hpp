#pragma once

#include <bgfx/bgfx.h>

#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <renderer/material.hpp>
#include <utility>
#include <variant>
#include <vector>

#include "core/sys/log.hpp"
#include "core/types.hpp"
#include "renderer/vertex_layout.hpp"

template <typename VL>
class Primitive
{
   public:
    using indice_type = uint16_t;
    using layout_type = VL;

    Primitive(
        std::vector<layout_type>  aVertices,
        std::vector<indice_type>  aIndices,
        std::unique_ptr<Material> aMaterial)
        : mVertices(std::move(aVertices)),
          mIndices(std::move(aIndices)),
          mMaterial(std::move(aMaterial)),
          mIsInitialized(false)
    {
        InitializePrimitive();
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
        BX_ASSERT(!mVertices.empty(), "primitive has no vertices");
        BX_ASSERT(!mIndices.empty(), "primitive has no indices");

        if (mIsInitialized) {
            return;
        }

        const bgfx::VertexLayout vertexLayout = layout_type::GetVertexLayout();

        DBG("initializing primitive with {} vertices and {} vertex layout data size",
            mVertices.size(),
            sizeof(layout_type));

        mVertexBufferHandle = bgfx::createVertexBuffer(
            bgfx::makeRef(mVertices.data(), SafeU32(sizeof(layout_type)) * mVertices.size()),
            vertexLayout);
        mIndexBufferHandle = bgfx::createIndexBuffer(
            bgfx::makeRef(mIndices.data(), SafeU32(sizeof(indice_type)) * mIndices.size()));

        mIsInitialized = true;
    }

   protected:
    std::vector<layout_type>  mVertices;
    std::vector<indice_type>  mIndices;
    std::unique_ptr<Material> mMaterial;

    bool mIsInitialized;

    bgfx::VertexBufferHandle mVertexBufferHandle;
    bgfx::IndexBufferHandle  mIndexBufferHandle;

   private:
    virtual void destroyPrimitive()
    {
        if (mIsInitialized) {
            TRACE(
                "destroying vertex buffer {} and index buffer {}",
                mVertexBufferHandle.idx,
                mIndexBufferHandle.idx);
            bgfx::destroy(mVertexBufferHandle);
            bgfx::destroy(mIndexBufferHandle);
        }
    }
    friend struct fmt::formatter<Primitive<VL>>;
};

using PrimitiveVariant = std::variant<
    std::unique_ptr<Primitive<PositionNormalUvVertex>>,
    std::unique_ptr<Primitive<PositionNormalUvBoneVertex>>,
    std::unique_ptr<Primitive<PositionVertex>>>;

template <typename VL>
struct fmt::formatter<Primitive<VL>> : fmt::formatter<std::string> {
    auto format(const Primitive<VL>& aObj, format_context& aCtx) const -> decltype(aCtx.out())
    {
        return fmt::format_to(
            aCtx.out(),
            "{} vertices, {} indices\nvertices: {}, indices: {}",
            aObj.mVertices.size(),
            aObj.mIndices.size(),
            aObj.mVertices,
            aObj.mIndices);
    }
};
