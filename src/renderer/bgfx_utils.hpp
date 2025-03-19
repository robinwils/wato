/*
 * Copyright 2011-2023 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#ifndef BGFX_UTILS_H_HEADER_GUARD
#define BGFX_UTILS_H_HEADER_GUARD

#include <bgfx/bgfx.h>
#include <bimg/bimg.h>
#include <bx/bounds.h>
#include <bx/file.h>
#include <bx/pixelformat.h>
#include <bx/string.h>
#include <tinystl/allocator.h>
#include <tinystl/vector.h>

struct BxSingleton {
    static BxSingleton& GetInstance()
    {
        static BxSingleton instance;

        return instance;
    }

    BxSingleton(BxSingleton const&)    = delete;
    void operator=(BxSingleton const&) = delete;

    bx::DefaultAllocator Allocator;
    bx::FileReader       Reader;
    bx::FileWriter       Writer;

   private:
    BxSingleton() {};
};

///
void unload(void* aPtr);

///
bimg::ImageContainer* imageLoad(const char* aFilePath, bgfx::TextureFormat::Enum aDstFormat);

///
void calcTangents(void* aVertices,
    uint16_t            aNumVertices,
    bgfx::VertexLayout  aLayout,
    const uint16_t*     aIndices,
    uint32_t            aNumIndices);

/// Returns true if both internal transient index and vertex buffer have
/// enough space.
///
/// @param[in] _numVertices Number of vertices.
/// @param[in] _layout Vertex layout.
/// @param[in] _numIndices Number of indices.
///
inline bool checkAvailTransientBuffers(uint32_t aNumVertices,
    const bgfx::VertexLayout&                   aLayout,
    uint32_t                                    aNumIndices)
{
    return aNumVertices == bgfx::getAvailTransientVertexBuffer(aNumVertices, aLayout)
           && (0 == aNumIndices || aNumIndices == bgfx::getAvailTransientIndexBuffer(aNumIndices));
}

///
inline uint32_t encodeNormalRgba8(float aX, float aY = 0.0f, float aZ = 0.0f, float aW = 0.0f)
{
    const float src[] = {
        aX * 0.5f + 0.5f,
        aY * 0.5f + 0.5f,
        aZ * 0.5f + 0.5f,
        aW * 0.5f + 0.5f,
    };
    uint32_t dst;
    bx::packRgba8(&dst, src);
    return dst;
}

///
struct MeshState {
    struct Texture {
        uint32_t            Flags;
        bgfx::UniformHandle Sampler;
        bgfx::TextureHandle Tex;
        uint8_t             Stage;
    };

    Texture             Textures[4];
    uint64_t            State;
    bgfx::ProgramHandle Program;
    uint8_t             NumTextures;
    bgfx::ViewId        ViewID;
};

/// bgfx::RendererType::Enum to name.
bx::StringView getName(bgfx::RendererType::Enum aType);

/// Name to bgfx::RendererType::Enum.
bgfx::RendererType::Enum getType(const bx::StringView& aName);

///
struct Args {
    Args(int aArgc, const char* const* aArgv);

    bgfx::RendererType::Enum MType;
    uint16_t                 MPciId;
};

#endif  // BGFX_UTILS_H_HEADER_GUARD
