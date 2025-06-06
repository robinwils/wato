#include "core/window.hpp"

#include <GLFW/glfw3.h>
#include <bx/bx.h>
#include <spdlog/spdlog.h>

#include <glm/ext/matrix_projection.hpp>

#if BX_PLATFORM_LINUX
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_WAYLAND
#define GLFW_EXPOSE_NATIVE_GLX
#elif BX_PLATFORM_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#elif BX_PLATFORM_OSX
#define GLFW_EXPOSE_NATIVE_COCOA
#define GLFW_EXPOSE_NATIVE_NSGL
#endif
#include <GLFW/glfw3native.h>

#include <stdexcept>

Button::Action to_action(int32_t aAction)
{
    switch (aAction) {
        case GLFW_PRESS:
            return Button::Action::Press;
        case GLFW_RELEASE:
            return Button::Action::Release;
        case GLFW_REPEAT:
            return Button::Action::Repeat;
        default:
            return Button::Action::Unknown;
    }
}

Keyboard::Key to_key(int32_t aKey)
{
    switch (aKey) {
        case GLFW_KEY_SPACE:
            return Keyboard::Key::Space;
        case GLFW_KEY_APOSTROPHE:
            return Keyboard::Key::Apostrophe;
        case GLFW_KEY_COMMA:
            return Keyboard::Key::Comma;
        case GLFW_KEY_MINUS:
            return Keyboard::Key::Minus;
        case GLFW_KEY_PERIOD:
            return Keyboard::Key::Period;
        case GLFW_KEY_SLASH:
            return Keyboard::Key::Slash;
        case GLFW_KEY_0:
            return Keyboard::Key::Num0;
        case GLFW_KEY_1:
            return Keyboard::Key::Num1;
        case GLFW_KEY_2:
            return Keyboard::Key::Num2;
        case GLFW_KEY_3:
            return Keyboard::Key::Num3;
        case GLFW_KEY_4:
            return Keyboard::Key::Num4;
        case GLFW_KEY_5:
            return Keyboard::Key::Num5;
        case GLFW_KEY_6:
            return Keyboard::Key::Num6;
        case GLFW_KEY_7:
            return Keyboard::Key::Num7;
        case GLFW_KEY_8:
            return Keyboard::Key::Num8;
        case GLFW_KEY_9:
            return Keyboard::Key::Num9;
        case GLFW_KEY_SEMICOLON:
            return Keyboard::Key::Semicolon;
        case GLFW_KEY_EQUAL:
            return Keyboard::Key::Equal;
        case GLFW_KEY_A:
            return Keyboard::Key::A;
        case GLFW_KEY_B:
            return Keyboard::Key::B;
        case GLFW_KEY_C:
            return Keyboard::Key::C;
        case GLFW_KEY_D:
            return Keyboard::Key::D;
        case GLFW_KEY_E:
            return Keyboard::Key::E;
        case GLFW_KEY_F:
            return Keyboard::Key::F;
        case GLFW_KEY_G:
            return Keyboard::Key::G;
        case GLFW_KEY_H:
            return Keyboard::Key::H;
        case GLFW_KEY_I:
            return Keyboard::Key::I;
        case GLFW_KEY_J:
            return Keyboard::Key::J;
        case GLFW_KEY_K:
            return Keyboard::Key::K;
        case GLFW_KEY_L:
            return Keyboard::Key::L;
        case GLFW_KEY_M:
            return Keyboard::Key::M;
        case GLFW_KEY_N:
            return Keyboard::Key::N;
        case GLFW_KEY_O:
            return Keyboard::Key::O;
        case GLFW_KEY_P:
            return Keyboard::Key::P;
        case GLFW_KEY_Q:
            return Keyboard::Key::Q;
        case GLFW_KEY_R:
            return Keyboard::Key::R;
        case GLFW_KEY_S:
            return Keyboard::Key::S;
        case GLFW_KEY_T:
            return Keyboard::Key::T;
        case GLFW_KEY_U:
            return Keyboard::Key::U;
        case GLFW_KEY_V:
            return Keyboard::Key::V;
        case GLFW_KEY_W:
            return Keyboard::Key::W;
        case GLFW_KEY_X:
            return Keyboard::Key::X;
        case GLFW_KEY_Y:
            return Keyboard::Key::Y;
        case GLFW_KEY_Z:
            return Keyboard::Key::Z;
        case GLFW_KEY_LEFT_BRACKET:
            return Keyboard::Key::LeftBracket;
        case GLFW_KEY_BACKSLASH:
            return Keyboard::Key::Backslash;
        case GLFW_KEY_RIGHT_BRACKET:
            return Keyboard::Key::RightBracket;
        case GLFW_KEY_GRAVE_ACCENT:
            return Keyboard::Key::GraveAccent;
        case GLFW_KEY_WORLD_1:
            return Keyboard::Key::World1;
        case GLFW_KEY_WORLD_2:
            return Keyboard::Key::World2;
        case GLFW_KEY_ESCAPE:
            return Keyboard::Key::Escape;
        case GLFW_KEY_ENTER:
            return Keyboard::Key::Enter;
        case GLFW_KEY_TAB:
            return Keyboard::Key::Tab;
        case GLFW_KEY_BACKSPACE:
            return Keyboard::Key::Backspace;
        case GLFW_KEY_INSERT:
            return Keyboard::Key::Insert;
        case GLFW_KEY_DELETE:
            return Keyboard::Key::Delete;
        case GLFW_KEY_RIGHT:
            return Keyboard::Key::Right;
        case GLFW_KEY_LEFT:
            return Keyboard::Key::Left;
        case GLFW_KEY_DOWN:
            return Keyboard::Key::Down;
        case GLFW_KEY_UP:
            return Keyboard::Key::Up;
        case GLFW_KEY_PAGE_UP:
            return Keyboard::Key::PageUp;
        case GLFW_KEY_PAGE_DOWN:
            return Keyboard::Key::PageDown;
        case GLFW_KEY_HOME:
            return Keyboard::Key::Home;
        case GLFW_KEY_END:
            return Keyboard::Key::End;
        case GLFW_KEY_CAPS_LOCK:
            return Keyboard::Key::CapsLock;
        case GLFW_KEY_SCROLL_LOCK:
            return Keyboard::Key::ScrollLock;
        case GLFW_KEY_NUM_LOCK:
            return Keyboard::Key::NumLock;
        case GLFW_KEY_PRINT_SCREEN:
            return Keyboard::Key::PrintScreen;
        case GLFW_KEY_PAUSE:
            return Keyboard::Key::Pause;
        case GLFW_KEY_F1:
            return Keyboard::Key::F1;
        case GLFW_KEY_F2:
            return Keyboard::Key::F2;
        case GLFW_KEY_F3:
            return Keyboard::Key::F3;
        case GLFW_KEY_F4:
            return Keyboard::Key::F4;
        case GLFW_KEY_F5:
            return Keyboard::Key::F5;
        case GLFW_KEY_F6:
            return Keyboard::Key::F6;
        case GLFW_KEY_F7:
            return Keyboard::Key::F7;
        case GLFW_KEY_F8:
            return Keyboard::Key::F8;
        case GLFW_KEY_F9:
            return Keyboard::Key::F9;
        case GLFW_KEY_F10:
            return Keyboard::Key::F10;
        case GLFW_KEY_F11:
            return Keyboard::Key::F11;
        case GLFW_KEY_F12:
            return Keyboard::Key::F12;
        case GLFW_KEY_F13:
            return Keyboard::Key::F13;
        case GLFW_KEY_F14:
            return Keyboard::Key::F14;
        case GLFW_KEY_F15:
            return Keyboard::Key::F15;
        case GLFW_KEY_F16:
            return Keyboard::Key::F16;
        case GLFW_KEY_F17:
            return Keyboard::Key::F17;
        case GLFW_KEY_F18:
            return Keyboard::Key::F18;
        case GLFW_KEY_F19:
            return Keyboard::Key::F19;
        case GLFW_KEY_F20:
            return Keyboard::Key::F20;
        case GLFW_KEY_F21:
            return Keyboard::Key::F21;
        case GLFW_KEY_F22:
            return Keyboard::Key::F22;
        case GLFW_KEY_F23:
            return Keyboard::Key::F23;
        case GLFW_KEY_F24:
            return Keyboard::Key::F24;
        case GLFW_KEY_F25:
            return Keyboard::Key::F25;
        case GLFW_KEY_KP_0:
            return Keyboard::Key::Kp0;
        case GLFW_KEY_KP_1:
            return Keyboard::Key::Kp1;
        case GLFW_KEY_KP_2:
            return Keyboard::Key::Kp2;
        case GLFW_KEY_KP_3:
            return Keyboard::Key::Kp3;
        case GLFW_KEY_KP_4:
            return Keyboard::Key::Kp4;
        case GLFW_KEY_KP_5:
            return Keyboard::Key::Kp5;
        case GLFW_KEY_KP_6:
            return Keyboard::Key::Kp6;
        case GLFW_KEY_KP_7:
            return Keyboard::Key::Kp7;
        case GLFW_KEY_KP_8:
            return Keyboard::Key::Kp8;
        case GLFW_KEY_KP_9:
            return Keyboard::Key::Kp9;
        case GLFW_KEY_KP_DECIMAL:
            return Keyboard::Key::KpDecimal;
        case GLFW_KEY_KP_DIVIDE:
            return Keyboard::Key::KpDivide;
        case GLFW_KEY_KP_MULTIPLY:
            return Keyboard::Key::KpMultiply;
        case GLFW_KEY_KP_SUBTRACT:
            return Keyboard::Key::KpSubtract;
        case GLFW_KEY_KP_ADD:
            return Keyboard::Key::KpAdd;
        case GLFW_KEY_KP_ENTER:
            return Keyboard::Key::KpEnter;
        case GLFW_KEY_KP_EQUAL:
            return Keyboard::Key::KpEqual;
        case GLFW_KEY_LEFT_SHIFT:
            return Keyboard::Key::LeftShift;
        case GLFW_KEY_LEFT_CONTROL:
            return Keyboard::Key::LeftControl;
        case GLFW_KEY_LEFT_ALT:
            return Keyboard::Key::LeftAlt;
        case GLFW_KEY_LEFT_SUPER:
            return Keyboard::Key::LeftSuper;
        case GLFW_KEY_RIGHT_SHIFT:
            return Keyboard::Key::RightShift;
        case GLFW_KEY_RIGHT_CONTROL:
            return Keyboard::Key::RightControl;
        case GLFW_KEY_RIGHT_ALT:
            return Keyboard::Key::RightAlt;
        case GLFW_KEY_RIGHT_SUPER:
            return Keyboard::Key::RightSuper;
        case GLFW_KEY_MENU:
            return Keyboard::Key::Menu;
        default:
            return Keyboard::Key::Count;
    }
}

Mouse::Button to_mouse_button(int32_t aButton)
{
    switch (aButton) {
        case GLFW_MOUSE_BUTTON_LEFT:
            return Mouse::Button::Left;
        case GLFW_MOUSE_BUTTON_RIGHT:
            return Mouse::Button::Right;
        case GLFW_MOUSE_BUTTON_MIDDLE:
            return Mouse::Button::Middle;
        default:
            return Mouse::Button::Unknown;
    }
}

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

    glfwSetKeyCallback(mGLFWWindow.get(), keyCallback);
    // glfwSetCharCallback(m_window[0], charCb);
    glfwSetScrollCallback(mGLFWWindow.get(), scrollCallback);
    glfwSetCursorPosCallback(mGLFWWindow.get(), cursorPosCallback);
    glfwSetMouseButtonCallback(mGLFWWindow.get(), mouseButtonCallback);
    // glfwSetWindowSizeCallback(m_window[0], windowSizeCb);
    // glfwSetDropCallback(m_window[0], dropFileCb);

    glfwSetWindowUserPointer(mGLFWWindow.get(), this);
    mIsInit = true;
}

void* WatoWindow::GetNativeDisplay()
{
#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
    void* display = glfwGetX11Display();
    if (display == nullptr) {
        spdlog::warn("cannot get X11 display, trying wayland");
        display  = glfwGetWaylandDisplay();
        mWayland = mWayland || (display != nullptr);
    }
    return display;
#else
    return nullptr;
#endif
}

void* WatoWindow::GetNativeWindow()
{
#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
    void* native =
        reinterpret_cast<void*>(static_cast<uintptr_t>(glfwGetX11Window(mGLFWWindow.get())));
    if (nullptr == native) {
        spdlog::warn("cannot get X11 native window, trying wayland");
        native   = glfwGetWaylandWindow(mGLFWWindow.get());
        mWayland = mWayland || (native != nullptr);
    }
    return native;
#elif BX_PLATFORM_OSX
    return glfwGetCocoaWindow(mGLFWWindow.get());
#elif BX_PLATFORM_WINDOWS
    return glfwGetWin32Window(mGLFWWindow.get());
#else
    return nullptr;
#endif
}

std::pair<glm::vec3, glm::vec3> WatoWindow::MouseUnproject(
    const Camera&    aCam,
    const glm::vec3& aCamPos) const
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

void WatoWindow::keyCallback(
    GLFWwindow* aWindow,
    int32_t     aKey,
    int32_t     aScancode,
    int32_t     aAction,
    int32_t     aMods)
{
    auto*         win   = static_cast<WatoWindow*>(glfwGetWindowUserPointer(aWindow));
    Input&        input = win->GetInput();
    Keyboard::Key key   = to_key(aKey);

    // don't reset current state, it will discard input and make movement choppy
    input.KeyboardState.SetKey(key, to_action(aAction));
    // fmt::print("key {} is {}\n", key_string(key), to_string(to_action(aAction)));

    if (aMods & GLFW_MOD_SHIFT) {
        input.KeyboardState.SetKeyModifier(key, ModifierKey::Shift);
    }
    if (aMods & GLFW_MOD_CONTROL) {
        input.KeyboardState.SetKeyModifier(key, ModifierKey::Ctrl);
    }
    if (aMods & GLFW_MOD_ALT) {
        input.KeyboardState.SetKeyModifier(key, ModifierKey::Alt);
    }
    if (aMods & GLFW_MOD_SUPER) {
        input.KeyboardState.SetKeyModifier(key, ModifierKey::Super);
    }
    if (aMods & GLFW_MOD_CAPS_LOCK) {
        input.KeyboardState.SetKeyModifier(key, ModifierKey::CapsLock);
    }
    if (aMods & GLFW_MOD_NUM_LOCK) {
        input.KeyboardState.SetKeyModifier(key, ModifierKey::NumLock);
    }
}

void WatoWindow::cursorPosCallback(GLFWwindow* aWindow, double aX, double aY)
{
    auto*  win   = static_cast<WatoWindow*>(glfwGetWindowUserPointer(aWindow));
    Input& input = win->GetInput();

    input.MouseState.Pos.x = aX;
    input.MouseState.Pos.y = aY;
}

void WatoWindow::mouseButtonCallback(
    GLFWwindow* aWindow,
    int32_t     aButton,
    int32_t     aAction,
    int32_t     aMods)
{
    auto*         win    = static_cast<WatoWindow*>(glfwGetWindowUserPointer(aWindow));
    Input&        input  = win->GetInput();
    Mouse::Button button = to_mouse_button(aButton);

    input.MouseState.SetKey(button, to_action(aAction));

    if (aMods & GLFW_MOD_SHIFT) {
        input.MouseState.SetKeyModifier(button, ModifierKey::Shift);
    }
    if (aMods & GLFW_MOD_CONTROL) {
        input.MouseState.SetKeyModifier(button, ModifierKey::Ctrl);
    }
    if (aMods & GLFW_MOD_ALT) {
        input.MouseState.SetKeyModifier(button, ModifierKey::Alt);
    }
    if (aMods & GLFW_MOD_SUPER) {
        input.MouseState.SetKeyModifier(button, ModifierKey::Super);
    }
    if (aMods & GLFW_MOD_CAPS_LOCK) {
        input.MouseState.SetKeyModifier(button, ModifierKey::CapsLock);
    }
    if (aMods & GLFW_MOD_NUM_LOCK) {
        input.MouseState.SetKeyModifier(button, ModifierKey::NumLock);
    }
    // spdlog::info("mouse state {}", input.MouseState.String());
    // spdlog::info("prev mouse state {}", input.PrevMouseState.String());
}

void WatoWindow::scrollCallback(GLFWwindow* aWindow, double aXoffset, double aYoffset)
{
    auto*  win   = static_cast<WatoWindow*>(glfwGetWindowUserPointer(aWindow));
    Input& input = win->GetInput();

    input.MouseState.Scroll.x = aXoffset;
    input.MouseState.Scroll.y = aYoffset;
}
