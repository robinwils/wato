#include "core/window.hpp"

#include <GLFW/glfw3.h>
#include <bx/bx.h>

#include <glm/ext/matrix_projection.hpp>

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

std::pair<glm::vec3, glm::vec3> WatoWindow::MouseUnproject(const Camera& aCam,
    const glm::vec3&                                                     aCamPos) const
{
    const MouseState& mouseState = mInput.MouseState;
    const float       x          = mouseState.Pos.x;
    const float       y          = Height<float>() - mouseState.Pos.y;
    const glm::mat4&  view       = aCam.View(aCamPos);
    const glm::mat4&  proj       = aCam.Projection(Width<float>(), Height<float>());
    const auto&       viewport   = glm::vec4(0, 0, Width<float>(), Height<float>());
    const glm::vec3&  near       = glm::unProject(glm::vec3(x, y, 0.0f), view, proj, viewport);
    const glm::vec3   far        = glm::unProject(glm::vec3(x, y, 1.0f), view, proj, viewport);

    return std::make_pair(near, far);
}
