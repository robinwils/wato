#pragma once

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include <memory>

#include "core/queue/ring_buffer.hpp"
#include "input/input.hpp"

class WatoWindow
{
   public:
    WatoWindow(int aWidth, int aHeight)
        : mWidth(aWidth), mHeight(aHeight), mIsInit(false), mWayland(false)
    {
    }

    WatoWindow(const WatoWindow&)            = delete;
    WatoWindow& operator=(const WatoWindow&) = delete;

    void Init();

    bool ShouldClose() { return glfwWindowShouldClose(mGLFWWindow.get()); }
    bool Resize()
    {
        int oldWidth = mWidth, oldHeight = mHeight;
        glfwGetWindowSize(mGLFWWindow.get(), &mWidth, &mHeight);
        return (mWidth != oldWidth || mHeight != oldHeight);
    }
    void  PollEvents() { glfwPollEvents(); }
    void* GetNativeDisplay();
    void* GetNativeWindow();

    template <typename T>
    T Width() const
    {
        return static_cast<T>(mWidth);
    }

    template <typename T>
    T Height() const
    {
        return static_cast<T>(mHeight);
    }

    Input& GetInput() { return mInput; }

    void SetSize(int aWidth, int aHeight)
    {
        mWidth  = aWidth;
        mHeight = aHeight;
    }

    [[nodiscard]] constexpr bool IsInitialized() const noexcept { return mIsInit; }
    [[nodiscard]] constexpr bool UseWayland() const noexcept { return mWayland; }

    [[nodiscard]] std::pair<glm::vec3, glm::vec3> MouseUnproject(
        const Camera&    aCam,
        const glm::vec3& aCamPos) const;

   private:
    struct GLFWwindowDeleter {
        void operator()(GLFWwindow* aWin) const noexcept
        {
            if (aWin) {
                glfwDestroyWindow(aWin);
            }
        }
    };
    using glfw_window_ptr = std::unique_ptr<GLFWwindow, GLFWwindowDeleter>;

    // raw callback handlers
    static void keyCallback(
        GLFWwindow* aWindow,
        int32_t     aKey,
        int32_t     aScancode,
        int32_t     aAction,
        int32_t     aMods);

    static void scrollCallback(GLFWwindow* aWindow, double aXoffset, double aYoffset);
    static void cursorPosCallback(GLFWwindow* aWindow, double aXpos, double aYpos);
    static void
    mouseButtonCallback(GLFWwindow* aWindow, int32_t aButton, int32_t aAction, int32_t aMods);
    static void errorCallback(int aCode, const char* aDescription)
    {
        spdlog::error("GLFW error [{}]: {}", aCode, aDescription);
    }

    int mWidth;
    int mHeight;

    glfw_window_ptr mGLFWWindow;

    Input mInput;

    bool mIsInit;
    bool mWayland;
};
