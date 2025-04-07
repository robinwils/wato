#include "core/window.hpp"

#include <GLFW/glfw3.h>
#include <bx/bx.h>

#if BX_PLATFORM_LINUX
#define GLFW_EXPOSE_NATIVE_X11
#elif BX_PLATFORM_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#elif BX_PLATFORM_OSX
#define GLFW_EXPOSE_NATIVE_COCOA
#endif
#include <GLFW/glfw3native.h>

#include <stdexcept>

void WatoWindow::Init()
{
    if (!glfwInit()) {
        throw std::runtime_error("failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    mGLFWWindow = glfw_window_ptr{glfwCreateWindow(mWidth, mHeight, "wato", nullptr, nullptr)};
    if (!mGLFWWindow) {
        glfwTerminate();
        throw std::runtime_error("failed to initialize GLFW window");
    }

    glfwMakeContextCurrent(mGLFWWindow.get());

    glfwSetKeyCallback(mGLFWWindow.get(), Input::KeyCallback);
    // glfwSetCharCallback(m_window[0], charCb);
    glfwSetScrollCallback(mGLFWWindow.get(), Input::ScrollCallback);
    glfwSetCursorPosCallback(mGLFWWindow.get(), Input::CursorPosCallback);
    glfwSetMouseButtonCallback(mGLFWWindow.get(), Input::MouseButtonCallback);
    // glfwSetWindowSizeCallback(m_window[0], windowSizeCb);
    // glfwSetDropCallback(m_window[0], dropFileCb);

    glfwSetWindowUserPointer(mGLFWWindow.get(), this);
    mIsInit = true;
}

void* WatoWindow::GetNativeDisplay()
{
#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
    return glfwGetX11Display();
#else
    return nullptr;
#endif
}

void* WatoWindow::GetNativeWindow()
{
#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
    return reinterpret_cast<void*>(static_cast<uintptr_t>(glfwGetX11Window(mGLFWWindow.get())));
#elif BX_PLATFORM_OSX
    return glfwGetCocoaWindow(mGLFWWindow.get());
#elif BX_PLATFORM_WINDOWS
    return glfwGetWin32Window(mGLFWWindow.get());
#else
    return nullptr;
#endif
}
