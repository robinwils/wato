#include "resource/texture_loader.hpp"

#include <bgfx/bgfx.h>
#include <fmt/core.h>

#include <memory>

#include "core/sys/log.hpp"
#include "core/sys/mem.hpp"
#include "resource/asset.hpp"

inline static void imageReleaseCb(void* aPtr, void* aUserData)
{
    BX_UNUSED(aPtr);
    auto* imageContainer = static_cast<bimg::ImageContainer*>(aUserData);
    bimg::imageFree(imageContainer);
}

TextureLoader::result_type TextureLoader::operator()(
    const char*              aName,
    uint64_t                 aFlags,
    uint8_t                  aSkip,
    bgfx::TextureInfo*       aInfo,
    bimg::Orientation::Enum* aOrientation)
{
    BX_UNUSED(aSkip);
    static bx::DefaultAllocator allocator;
    bgfx::TextureHandle         handle = BGFX_INVALID_HANDLE;
    bx::FileReader              fr;

    uint32_t    size      = 0;
    std::string assetPath = FindAsset(aName);
    if (assetPath == "") {
        throw std::runtime_error(fmt::format("cannot find asset {}", aName));
    }

    void* data = load(&fr, &allocator, assetPath.c_str(), &size);
    if (nullptr != data) {
        bimg::ImageContainer* imageContainer = bimg::imageParse(&allocator, data, size);

        if (nullptr != imageContainer) {
            if (nullptr != aOrientation) {
                *aOrientation = imageContainer->m_orientation;
            }

            const bgfx::Memory* mem = bgfx::makeRef(
                imageContainer->m_data,
                imageContainer->m_size,
                imageReleaseCb,
                imageContainer);
            bx::free(&allocator, data);

            if (imageContainer->m_cubeMap) {
                handle = bgfx::createTextureCube(
                    uint16_t(imageContainer->m_width),
                    1 < imageContainer->m_numMips,
                    imageContainer->m_numLayers,
                    bgfx::TextureFormat::Enum(imageContainer->m_format),
                    aFlags,
                    mem);
            } else if (1 < imageContainer->m_depth) {
                handle = bgfx::createTexture3D(
                    uint16_t(imageContainer->m_width),
                    uint16_t(imageContainer->m_height),
                    uint16_t(imageContainer->m_depth),
                    1 < imageContainer->m_numMips,
                    bgfx::TextureFormat::Enum(imageContainer->m_format),
                    aFlags,
                    mem);
            } else if (bgfx::isTextureValid(
                           0,
                           false,
                           imageContainer->m_numLayers,
                           bgfx::TextureFormat::Enum(imageContainer->m_format),
                           aFlags)) {
                handle = bgfx::createTexture2D(
                    uint16_t(imageContainer->m_width),
                    uint16_t(imageContainer->m_height),
                    1 < imageContainer->m_numMips,
                    imageContainer->m_numLayers,
                    bgfx::TextureFormat::Enum(imageContainer->m_format),
                    aFlags,
                    mem);
            }

            if (bgfx::isValid(handle)) {
                spdlog::debug("Loaded texture {}", assetPath);
                bgfx::setName(handle, assetPath.c_str());
            }

            if (nullptr != aInfo) {
                bgfx::calcTextureSize(
                    *aInfo,
                    uint16_t(imageContainer->m_width),
                    uint16_t(imageContainer->m_height),
                    uint16_t(imageContainer->m_depth),
                    imageContainer->m_cubeMap,
                    1 < imageContainer->m_numMips,
                    imageContainer->m_numLayers,
                    bgfx::TextureFormat::Enum(imageContainer->m_format));
            }
        }
    }

    return std::make_shared<bgfx::TextureHandle>(handle);
}

TextureLoader::result_type TextureLoader::operator()(
    uint16_t                  aWidth,
    uint16_t                  aHeight,
    bool                      aHasMips,
    uint16_t                  aNumLayers,
    bgfx::TextureFormat::Enum aFormat,
    uint64_t                  aFlags,
    const void*               aMem)
{
    bgfx::TextureHandle handle = BGFX_INVALID_HANDLE;

    if (aMem == nullptr) {
        handle =
            bgfx::createTexture2D(aWidth, aHeight, aHasMips, aNumLayers, aFormat, aFlags, nullptr);
    } else {
        handle = bgfx::createTexture2D(
            aWidth,
            aHeight,
            aHasMips,
            aNumLayers,
            aFormat,
            aFlags,
            bgfx::copy(aMem, aWidth * aHeight));
    }

    if (bgfx::isValid(handle)) {
        spdlog::debug(
            "{}x{} {} texture created ",
            aWidth,
            aHeight,
            aMem == nullptr ? "mutable" : "immutable");
        bgfx::setName(handle, "buffer texture");
    }

    return std::make_shared<bgfx::TextureHandle>(handle);
}
