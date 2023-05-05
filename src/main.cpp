#include <iostream>
#include <GLFW/glfw3.h>
#include <bx/bx.h>
#include <bx/math.h>
#include <bx/allocator.h>
#include <bx/file.h>
#include <bx/debug.h>
#include <bx/timer.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#if BX_PLATFORM_LINUX
#define GLFW_EXPOSE_NATIVE_X11
#elif BX_PLATFORM_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#elif BX_PLATFORM_OSX
#define GLFW_EXPOSE_NATIVE_COCOA
#endif
#include <GLFW/glfw3native.h>

#include "imgui_helper.h"
#include "bgfx_utils.h"
#include <input/input.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <camera.hpp>
#include <primitive/plane.hpp>

#include <entt/entt.hpp>

BxFactory g_bxFactory;

struct PosColorVertex
{
    float m_x;
    float m_y;
    float m_z;
    uint32_t m_abgr;

    static void init()
    {
        ms_layout
            .begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
            .end();
    };

    static bgfx::VertexLayout ms_layout;
};

bgfx::VertexLayout PosColorVertex::ms_layout;

static PosColorVertex s_cubeVertices[] =
{
    {-1.0f,  1.0f,  1.0f, 0xff000000 },
    { 1.0f,  1.0f,  1.0f, 0xff0000ff },
    {-1.0f, -1.0f,  1.0f, 0xff00ff00 },
    { 1.0f, -1.0f,  1.0f, 0xff00ffff },
    {-1.0f,  1.0f, -1.0f, 0xffff0000 },
    { 1.0f,  1.0f, -1.0f, 0xffff00ff },
    {-1.0f, -1.0f, -1.0f, 0xffffff00 },
    { 1.0f, -1.0f, -1.0f, 0xffffffff },
};

static const uint16_t s_cubeTriList[] =
{
    0, 1, 2, // 0
    1, 3, 2,
    4, 6, 5, // 2
    5, 6, 7,
    0, 2, 4, // 4
    4, 2, 6,
    1, 5, 3, // 6
    5, 7, 3,
    0, 4, 1, // 8
    4, 5, 1,
    2, 3, 6, // 10
    6, 3, 7,
};

static const bgfx::Memory* loadMem(bx::FileReaderI* _reader, const char* _filePath)
{
    if (bx::open(_reader, _filePath))
    {
        uint32_t size = (uint32_t)bx::getSize(_reader);
        const bgfx::Memory* mem = bgfx::alloc(size + 1);
        bx::read(_reader, mem->data, size, bx::ErrorAssert{});
        bx::close(_reader);
        mem->data[mem->size - 1] = '\0';
        return mem;
    }

    DBG("Failed to load %s.", _filePath);
    return NULL;
}

static bgfx::ShaderHandle loadShader(bx::FileReaderI* _reader, const char* _name)
{
    char filePath[512];

    const char* renderer = "???";

    bx::strCopy(filePath, BX_COUNTOF(filePath), "shaders/");
    bx::strCat(filePath, BX_COUNTOF(filePath), _name);

#if BX_PLATFORM_WINDOWS
    bx::strCat(filePath, BX_COUNTOF(filePath), "_windows");
#endif

    switch (bgfx::getRendererType())
    {
    case bgfx::RendererType::Noop:
    case bgfx::RendererType::Direct3D9:  renderer = "_dx9";   break;
    case bgfx::RendererType::Direct3D11:
    case bgfx::RendererType::Direct3D12: renderer = "_dx11";  break;
    case bgfx::RendererType::Agc:
    case bgfx::RendererType::Gnm:        renderer = "_pssl";  break;
    case bgfx::RendererType::Metal:      renderer = "_metal"; break;
    case bgfx::RendererType::Nvn:        renderer = "_nvn";   break;
    case bgfx::RendererType::OpenGL:     renderer = "_glsl";  break;
    case bgfx::RendererType::OpenGLES:   renderer = "_essl";  break;
    case bgfx::RendererType::Vulkan:
    case bgfx::RendererType::WebGPU:     renderer = "_spirv"; break;

    case bgfx::RendererType::Count:
        BX_ASSERT(false, "You should not be here!");
        break;
    }

    bx::strCat(filePath, BX_COUNTOF(filePath), renderer);
    bx::strCat(filePath, BX_COUNTOF(filePath), ".bin");

    bgfx::ShaderHandle handle = bgfx::createShader(loadMem(_reader, filePath));
    bgfx::setName(handle, _name);

    return handle;
}


bgfx::ProgramHandle loadProgram(const char* _vsName, const char* _fsName)
{
    bgfx::ShaderHandle vsh = loadShader(g_bxFactory.getDefaultFileReader(), _vsName);
    bgfx::ShaderHandle fsh = BGFX_INVALID_HANDLE;
    if (NULL != _fsName)
    {
        fsh = loadShader(g_bxFactory.getDefaultFileReader(), _fsName);
    }

    return bgfx::createProgram(vsh, fsh, true /* destroy shaders when program is destroyed */);

}

int main()
{
    if (!glfwInit())
        return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //macos: glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(1024, 768, "wato", nullptr, nullptr);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Call bgfx::renderFrame before bgfx::init to signal to bgfx not to create a render thread.
    // Most graphics APIs must be used on the same thread that created the window.
    bgfx::renderFrame();
    bgfx::Init init;

#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
    init.platformData.ndt = glfwGetX11Display();
    init.platformData.nwh = (void*)(uintptr_t)glfwGetX11Window(window);
#elif BX_PLATFORM_OSX
    init.platformData.nwh = glfwGetCocoaWindow(window);
#elif BX_PLATFORM_WINDOWS
    init.platformData.nwh = glfwGetWin32Window(window);
#endif

    init.type = bgfx::RendererType::Vulkan;

    int width, height;
    glfwGetWindowSize(window, &width, &height);
    init.resolution.width = (uint32_t)width;
    init.resolution.height = (uint32_t)height;
    init.resolution.reset = BGFX_RESET_VSYNC;
    if (!bgfx::init(init))
        return 1;
    // Set view 0 to the same dimensions as the window and to clear the color buffer.
    const bgfx::ViewId kClearView = 0;
    
    // Enable stats or debug text.
    bgfx::setDebug(BGFX_DEBUG_TEXT);

    // Set view 0 clear state.
    bgfx::setViewClear(0
        , BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH
        , 0x303030ff
        , 1.0f
        , 0
    );

    // Create vertex stream declaration.
    PosColorVertex::init();

    // Create static vertex buffer.
    bgfx::VertexBufferHandle vbh = bgfx::createVertexBuffer(
        // Static data can be passed with bgfx::makeRef
        bgfx::makeRef(s_cubeVertices, sizeof(s_cubeVertices))
        , PosColorVertex::ms_layout
    );

    // Create static index buffer for triangle list rendering.
    bgfx::IndexBufferHandle ibh = bgfx::createIndexBuffer(
        // Static data can be passed with bgfx::makeRef
        bgfx::makeRef(s_cubeTriList, sizeof(s_cubeTriList))
    );

    bgfx::ProgramHandle program = loadProgram("vs_cubes", "fs_cubes");

    imguiCreate();
    Input input(window);
    input.init();
    glfwSetWindowUserPointer(window, &input);
    Camera camera;
    PlanePrimitive plane;
    double prevTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        // Handle window resize.
        int oldWidth = width, oldHeight = height;
        glfwGetWindowSize(window, &width, &height);
        if (width != oldWidth || height != oldHeight) {
            bgfx::reset((uint32_t)width, (uint32_t)height, BGFX_RESET_VSYNC);
            bgfx::setViewRect(kClearView, 0, 0, bgfx::BackbufferRatio::Equal);
        }
        bgfx::touch(kClearView);
        // Use debug font to print information about this example.
        bgfx::dbgTextClear();
        //bgfx::dbgTextImage(bx::max<uint16_t>(uint16_t(width / 2 / 8), 20) - 20, bx::max<uint16_t>(uint16_t(height / 2 / 16), 6) - 6, 40, 12, s_logo, 160);
        bgfx::dbgTextPrintf(0, 0, 0x0f, "Press F1 to toggle stats.");
        bgfx::dbgTextPrintf(0, 1, 0x0f, "Color can be changed with ANSI \x1b[9;me\x1b[10;ms\x1b[11;mc\x1b[12;ma\x1b[13;mp\x1b[14;me\x1b[0m code too.");
        bgfx::dbgTextPrintf(80, 1, 0x0f, "\x1b[;0m    \x1b[;1m    \x1b[; 2m    \x1b[; 3m    \x1b[; 4m    \x1b[; 5m    \x1b[; 6m    \x1b[; 7m    \x1b[0m");
        bgfx::dbgTextPrintf(80, 2, 0x0f, "\x1b[;8m    \x1b[;9m    \x1b[;10m    \x1b[;11m    \x1b[;12m    \x1b[;13m    \x1b[;14m    \x1b[;15m    \x1b[0m");
        const bgfx::Stats* stats = bgfx::getStats();
        bgfx::dbgTextPrintf(0, 2, 0x0f, "Backbuffer %dW x %dH in pixels, debug text %dW x %dH in characters.", stats->width, stats->height, stats->textWidth, stats->textHeight);

        imguiBeginFrame(input, uint16_t(width), uint16_t(height));

        showImguiDialogs(camera, input, width, height);

        imguiEndFrame();

        auto t = glfwGetTime();
        auto dt = t - prevTime;
        prevTime = t;


        camera.update(input, dt);
        const glm::mat4 view = camera.view();
        const glm::mat4 proj = camera.projection(float(width) / float(height));

        bgfx::setViewTransform(0, glm::value_ptr(view), glm::value_ptr(proj));
        bgfx::setViewRect(0, 0, 0, uint16_t(width), uint16_t(height));

        // This dummy draw call is here to make sure that view 0 is cleared
// if no other draw calls are submitted to view 0.
        bgfx::touch(0);

        uint64_t state = 0
            | BGFX_STATE_WRITE_R
            | BGFX_STATE_WRITE_G
            | BGFX_STATE_WRITE_B
            | BGFX_STATE_WRITE_A
            | BGFX_STATE_WRITE_Z
            | BGFX_STATE_DEPTH_TEST_LESS
            | BGFX_STATE_CULL_CW
            | BGFX_STATE_MSAA
            ;

        glm::mat4 plane_mtx = glm::mat4(1.0f) * glm::mat4(1.0f) * 4.0f * glm::scale(glm::mat4(1.0f), glm::vec3(4.0f));
        bgfx::setTransform(glm::value_ptr(plane_mtx));

        plane.submitPrimitive(program);

        // Submit 11x11 cubes.
        for (uint32_t yy = 0; yy < 1; ++yy)
        {
            for (uint32_t xx = 0; xx < 1; ++xx)
            {
                glm::mat4 scale_mtx = glm::mat4(1.0f);
                glm::mat4 translate_mtx = glm::translate(glm::mat4(1.0f),
                                                         glm::vec3(float(xx) * 3.0f, float(yy) * 3.0f, 0.0f));
                glm::mat4 rotate_mtx = glm::rotate(glm::mat4(1.0), (float) t, glm::vec3(xx, yy, 1.0f));
                glm::mat4 mtx = translate_mtx * rotate_mtx * scale_mtx;

                // Set model matrix for rendering.
                bgfx::setTransform(glm::value_ptr(mtx));

                // Set vertex and index buffer.
                bgfx::setVertexBuffer(0, vbh);
                bgfx::setIndexBuffer(ibh);

                // Set render states.
                bgfx::setState(state);

                // Submit primitive for rendering to view 0.
                bgfx::submit(0, program);
            }
        }

        // Advance to next frame. Process submitted rendering primitives.
        bgfx::frame();
    }

    return 0;
}
