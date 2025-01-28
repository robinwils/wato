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

#if defined(None) // X11 defines this...
#	undef None
#endif // defined(None)

#include <imgui_helper.h>
#include <renderer/bgfx_utils.hpp>
#include <input/input.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <core/camera.hpp>
#include <renderer/plane.hpp>

#include <core/registry.hpp>
#include <systems/render.hpp>

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
        , 0x0090cfff
        , 1.0f
        , 0
    );

    Registry registry;

    /*
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
    */

    imguiCreate();
    Input input(window);
    input.init();
    glfwSetWindowUserPointer(window, &input);
    Camera camera;

    registry.spawnLight();
    registry.spawnMap(20, 20);
    //registry.spawnPlane();
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

        renderSceneObjects(registry);

        imguiEndFrame();
        // Advance to next frame. Process submitted rendering primitives.
        bgfx::frame();
    }

    return 0;
}
