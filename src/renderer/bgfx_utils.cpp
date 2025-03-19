/*
 * Copyright 2011-2023 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#include <bx/math.h>
#include <bx/timer.h>
#include <tinystl/allocator.h>
#include <tinystl/string.h>
#include <tinystl/vector.h>
namespace stl = tinystl;

#include <bgfx/bgfx.h>
#include <bx/commandline.h>
#include <bx/endian.h>
#include <bx/file.h>
#include <bx/math.h>
#include <bx/readerwriter.h>
#include <bx/string.h>
// #include <meshoptimizer/src/meshoptimizer.h>

#include <bimg/decode.h>

#include <core/sys.hpp>
#include <renderer/bgfx_utils.hpp>

void calcTangents(void* aVertices,
    uint16_t            aNumVertices,
    bgfx::VertexLayout  aLayout,
    const uint16_t*     aIndices,
    uint32_t            aNumIndices)
{
    struct PosTexcoord {
        float X;
        float Y;
        float Z;
        float Pad0;
        float U;
        float V;
        float Pad1;
        float Pad2;
    };

    float* tangents = new float[6 * aNumVertices];
    bx::memSet(tangents, 0, 6 * aNumVertices * sizeof(float));

    PosTexcoord v0;
    PosTexcoord v1;
    PosTexcoord v2;

    for (uint32_t ii = 0, num = aNumIndices / 3; ii < num; ++ii) {
        const uint16_t* indices = &aIndices[ii * 3];
        uint32_t        i0      = indices[0];
        uint32_t        i1      = indices[1];
        uint32_t        i2      = indices[2];

        bgfx::vertexUnpack(&v0.X, bgfx::Attrib::Position, aLayout, aVertices, i0);
        bgfx::vertexUnpack(&v0.U, bgfx::Attrib::TexCoord0, aLayout, aVertices, i0);

        bgfx::vertexUnpack(&v1.X, bgfx::Attrib::Position, aLayout, aVertices, i1);
        bgfx::vertexUnpack(&v1.U, bgfx::Attrib::TexCoord0, aLayout, aVertices, i1);

        bgfx::vertexUnpack(&v2.X, bgfx::Attrib::Position, aLayout, aVertices, i2);
        bgfx::vertexUnpack(&v2.U, bgfx::Attrib::TexCoord0, aLayout, aVertices, i2);

        const float bax = v1.X - v0.X;
        const float bay = v1.Y - v0.Y;
        const float baz = v1.Z - v0.Z;
        const float bau = v1.U - v0.U;
        const float bav = v1.V - v0.V;

        const float cax = v2.X - v0.X;
        const float cay = v2.Y - v0.Y;
        const float caz = v2.Z - v0.Z;
        const float cau = v2.U - v0.U;
        const float cav = v2.V - v0.V;

        const float det    = (bau * cav - bav * cau);
        const float invDet = 1.0f / det;

        const float tx = (bax * cav - cax * bav) * invDet;
        const float ty = (bay * cav - cay * bav) * invDet;
        const float tz = (baz * cav - caz * bav) * invDet;

        const float bx = (cax * bau - bax * cau) * invDet;
        const float by = (cay * bau - bay * cau) * invDet;
        const float bz = (caz * bau - baz * cau) * invDet;

        for (uint32_t jj = 0; jj < 3; ++jj) {
            float* tanu  = &tangents[indices[jj] * 6];
            float* tanv  = &tanu[3];
            tanu[0]     += tx;
            tanu[1]     += ty;
            tanu[2]     += tz;

            tanv[0] += bx;
            tanv[1] += by;
            tanv[2] += bz;
        }
    }

    for (uint32_t ii = 0; ii < aNumVertices; ++ii) {
        const bx::Vec3 tanu = bx::load<bx::Vec3>(&tangents[ii * 6]);
        const bx::Vec3 tanv = bx::load<bx::Vec3>(&tangents[ii * 6 + 3]);

        float nxyzw[4];
        bgfx::vertexUnpack(nxyzw, bgfx::Attrib::Normal, aLayout, aVertices, ii);

        const bx::Vec3 normal = bx::load<bx::Vec3>(nxyzw);
        const float    ndt    = bx::dot(normal, tanu);
        const bx::Vec3 nxt    = bx::cross(normal, tanu);
        const bx::Vec3 tmp    = bx::sub(tanu, bx::mul(normal, ndt));

        float tangent[4];
        bx::store(tangent, bx::normalize(tmp));
        tangent[3] = bx::dot(nxt, tanv) < 0.0f ? -1.0f : 1.0f;

        bgfx::vertexPack(tangent, true, bgfx::Attrib::Tangent, aLayout, aVertices, ii);
    }

    delete[] tangents;
}

namespace bgfx
{
int32_t read(bx::ReaderI* aReader, bgfx::VertexLayout& aLayout, bx::Error* aErr);
}  // namespace bgfx

struct RendererTypeRemap {
    bx::StringView           Name;
    bgfx::RendererType::Enum Type;
};

static RendererTypeRemap sRendererTypeRemap[] = {
    {"d3d11", bgfx::RendererType::Direct3D11},
    {"d3d12", bgfx::RendererType::Direct3D12},
    {"gl",    bgfx::RendererType::OpenGL    },
    {"mtl",   bgfx::RendererType::Metal     },
    {"noop",  bgfx::RendererType::Noop      },
    {"vk",    bgfx::RendererType::Vulkan    },
};

bx::StringView getName(bgfx::RendererType::Enum aType)
{
    for (uint32_t ii = 0; ii < BX_COUNTOF(sRendererTypeRemap); ++ii) {
        const RendererTypeRemap& remap = sRendererTypeRemap[ii];

        if (aType == remap.Type) {
            return remap.Name;
        }
    }

    return "";
}

bgfx::RendererType::Enum getType(const bx::StringView& aName)
{
    for (uint32_t ii = 0; ii < BX_COUNTOF(sRendererTypeRemap); ++ii) {
        const RendererTypeRemap& remap = sRendererTypeRemap[ii];

        if (0 == bx::strCmpI(aName, remap.Name)) {
            return remap.Type;
        }
    }

    return bgfx::RendererType::Count;
}

Args::Args(int aArgc, const char* const* aArgv)
    : MType(bgfx::RendererType::Count), MPciId(BGFX_PCI_ID_NONE)
{
    bx::CommandLine cmdLine(aArgc, (const char**)aArgv);

    if (cmdLine.hasArg("gl")) {
        MType = bgfx::RendererType::OpenGL;
    } else if (cmdLine.hasArg("vk")) {
        MType = bgfx::RendererType::Vulkan;
    } else if (cmdLine.hasArg("noop")) {
        MType = bgfx::RendererType::Noop;
    }
    if (cmdLine.hasArg("d3d11")) {
        MType = bgfx::RendererType::Direct3D11;
    } else if (cmdLine.hasArg("d3d12")) {
        MType = bgfx::RendererType::Direct3D12;
    } else if (BX_ENABLED(BX_PLATFORM_OSX)) {
        if (cmdLine.hasArg("mtl")) {
            MType = bgfx::RendererType::Metal;
        }
    }

    if (cmdLine.hasArg("amd")) {
        MPciId = BGFX_PCI_ID_AMD;
    } else if (cmdLine.hasArg("nvidia")) {
        MPciId = BGFX_PCI_ID_NVIDIA;
    } else if (cmdLine.hasArg("intel")) {
        MPciId = BGFX_PCI_ID_INTEL;
    } else if (cmdLine.hasArg("sw")) {
        MPciId = BGFX_PCI_ID_SOFTWARE_RASTERIZER;
    }
}
