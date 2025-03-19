#include "renderer/material.hpp"

#include "core/sys.hpp"

bgfx::ShaderHandle loadShader(bx::FileReaderI* aReader, const char* aName)
{
    char filePath[512];

    const char* renderer = "???";

    bx::strCopy(filePath, BX_COUNTOF(filePath), "shaders/");
    bx::strCat(filePath, BX_COUNTOF(filePath), aName);

#if BX_PLATFORM_WINDOWS
    bx::strCat(filePath, BX_COUNTOF(filePath), "_windows");
#elif BX_PLATFORM_LINUX
    bx::strCat(filePath, BX_COUNTOF(filePath), "_linux");
#elif BX_PLATFORM_OSX
    bx::strCat(filePath, BX_COUNTOF(filePath), "_osx");
#endif

    switch (bgfx::getRendererType()) {
        case bgfx::RendererType::Noop:
        case bgfx::RendererType::Direct3D11:
        case bgfx::RendererType::Direct3D12:
            renderer = "_dx11";
            break;
        case bgfx::RendererType::Agc:
        case bgfx::RendererType::Gnm:
            renderer = "_pssl";
            break;
        case bgfx::RendererType::Metal:
            renderer = "_metal";
            break;
        case bgfx::RendererType::Nvn:
            renderer = "_nvn";
            break;
        case bgfx::RendererType::OpenGL:
            renderer = "_glsl";
            break;
        case bgfx::RendererType::OpenGLES:
            renderer = "_essl";
            break;
        case bgfx::RendererType::Vulkan:
            renderer = "_spirv";
            break;

        case bgfx::RendererType::Count:
            BX_ASSERT(false, "You should not be here!");
            break;
    }

    bx::strCat(filePath, BX_COUNTOF(filePath), renderer);
    bx::strCat(filePath, BX_COUNTOF(filePath), ".bin");

    bgfx::ShaderHandle handle = bgfx::createShader(loadMem(aReader, filePath));
    bgfx::setName(handle, aName);

    return handle;
}

bgfx::ProgramHandle loadProgram(bx::FileReader* aFr, const char* aVsName, const char* aFsName)
{
    bgfx::ShaderHandle vsh = loadShader(aFr, aVsName);
    bgfx::ShaderHandle fsh = BGFX_INVALID_HANDLE;
    if (NULL != aFsName) {
        fsh = loadShader(aFr, aFsName);
    }

    return bgfx::createProgram(vsh, fsh, true /* destroy shaders when program is destroyed */);
}
