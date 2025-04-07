#include "input.hpp"

#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <imgui.h>
#include <string.h>

#include <vector>

#include "components/camera.hpp"
#include "core/queue/ring_buffer.hpp"
#include "core/sys/log.hpp"
#include "core/window.hpp"
#include "glm/gtx/string_cast.hpp"

void MouseState::Clear()
{
    Pos.x    = -1.0f;
    Pos.y    = -1.0f;
    Scroll.x = 0.0f;
    Scroll.y = 0.0f;
    memset(Buttons, 0, N_BUTTONS);
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

std::string to_string(Button::Action aAction)
{
    switch (aAction) {
        case Button::Unknown:
            return "Unknown";
        case Button::Press:
            return "Press";
        case Button::Release:
            return "Release";
        case Button::Repeat:
            return "Repeat";
            break;
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

std::string key_string(const Keyboard::Key& aKey)
{
    switch (aKey) {
        case Keyboard::Space:
            return "Space";
        case Keyboard::Apostrophe:
            return "Apostrophe";
        case Keyboard::Comma:
            return "Comma";
        case Keyboard::Minus:
            return "Minus";
        case Keyboard::Period:
            return "Period";
        case Keyboard::Slash:
            return "Slash";
        case Keyboard::Num0:
            return "Num0";
        case Keyboard::Num1:
            return "Num1";
        case Keyboard::Num2:
            return "Num2";
        case Keyboard::Num3:
            return "Num3";
        case Keyboard::Num4:
            return "Num4";
        case Keyboard::Num5:
            return "Num5";
        case Keyboard::Num6:
            return "Num6";
        case Keyboard::Num7:
            return "Num7";
        case Keyboard::Num8:
            return "Num8";
        case Keyboard::Num9:
            return "Num9";
        case Keyboard::Semicolon:
            return "Semicolon";
        case Keyboard::Equal:
            return "Equal";
        case Keyboard::A:
            return "A";
        case Keyboard::B:
            return "B";
        case Keyboard::C:
            return "C";
        case Keyboard::D:
            return "D";
        case Keyboard::E:
            return "E";
        case Keyboard::F:
            return "F";
        case Keyboard::G:
            return "G";
        case Keyboard::H:
            return "H";
        case Keyboard::I:
            return "I";
        case Keyboard::J:
            return "J";
        case Keyboard::K:
            return "K";
        case Keyboard::L:
            return "L";
        case Keyboard::M:
            return "M";
        case Keyboard::N:
            return "N";
        case Keyboard::O:
            return "O";
        case Keyboard::P:
            return "P";
        case Keyboard::Q:
            return "Q";
        case Keyboard::R:
            return "R";
        case Keyboard::S:
            return "S";
        case Keyboard::T:
            return "T";
        case Keyboard::U:
            return "U";
        case Keyboard::V:
            return "V";
        case Keyboard::W:
            return "W";
        case Keyboard::X:
            return "X";
        case Keyboard::Y:
            return "Y";
        case Keyboard::Z:
            return "Z";
        case Keyboard::LeftBracket:
            return "LeftBracket";
        case Keyboard::Backslash:
            return "Backslash";
        case Keyboard::RightBracket:
            return "RightBracket";
        case Keyboard::GraveAccent:
            return "GraveAccent";
        case Keyboard::World1:
            return "World1";
        case Keyboard::World2:
            return "World2";
        case Keyboard::Escape:
            return "Escape";
        case Keyboard::Enter:
            return "Enter";
        case Keyboard::Tab:
            return "Tab";
        case Keyboard::Backspace:
            return "Backspace";
        case Keyboard::Insert:
            return "Insert";
        case Keyboard::Delete:
            return "Delete";
        case Keyboard::Right:
            return "Right";
        case Keyboard::Left:
            return "Left";
        case Keyboard::Down:
            return "Down";
        case Keyboard::Up:
            return "Up";
        case Keyboard::PageUp:
            return "PageUp";
        case Keyboard::PageDown:
            return "PageDown";
        case Keyboard::Home:
            return "Home";
        case Keyboard::End:
            return "End";
        case Keyboard::CapsLock:
            return "CapsLock";
        case Keyboard::ScrollLock:
            return "ScrollLock";
        case Keyboard::NumLock:
            return "NumLock";
        case Keyboard::PrintScreen:
            return "PrintScreen";
        case Keyboard::Pause:
            return "Pause";
        case Keyboard::F1:
            return "F1";
        case Keyboard::F2:
            return "F2";
        case Keyboard::F3:
            return "F3";
        case Keyboard::F4:
            return "F4";
        case Keyboard::F5:
            return "F5";
        case Keyboard::F6:
            return "F6";
        case Keyboard::F7:
            return "F7";
        case Keyboard::F8:
            return "F8";
        case Keyboard::F9:
            return "F9";
        case Keyboard::F10:
            return "F10";
        case Keyboard::F11:
            return "F11";
        case Keyboard::F12:
            return "F12";
        case Keyboard::F13:
            return "F13";
        case Keyboard::F14:
            return "F14";
        case Keyboard::F15:
            return "F15";
        case Keyboard::F16:
            return "F16";
        case Keyboard::F17:
            return "F17";
        case Keyboard::F18:
            return "F18";
        case Keyboard::F19:
            return "F19";
        case Keyboard::F20:
            return "F20";
        case Keyboard::F21:
            return "F21";
        case Keyboard::F22:
            return "F22";
        case Keyboard::F23:
            return "F23";
        case Keyboard::F24:
            return "F24";
        case Keyboard::F25:
            return "F25";
        case Keyboard::Kp0:
            return "Kp0";
        case Keyboard::Kp1:
            return "Kp1";
        case Keyboard::Kp2:
            return "Kp2";
        case Keyboard::Kp3:
            return "Kp3";
        case Keyboard::Kp4:
            return "Kp4";
        case Keyboard::Kp5:
            return "Kp5";
        case Keyboard::Kp6:
            return "Kp6";
        case Keyboard::Kp7:
            return "Kp7";
        case Keyboard::Kp8:
            return "Kp8";
        case Keyboard::Kp9:
            return "Kp9";
        case Keyboard::KpDecimal:
            return "KpDecimal";
        case Keyboard::KpDivide:
            return "KpDivide";
        case Keyboard::KpMultiply:
            return "KpMultiply";
        case Keyboard::KpSubtract:
            return "KpSubtract";
        case Keyboard::KpAdd:
            return "KpAdd";
        case Keyboard::KpEnter:
            return "KpEnter";
        case Keyboard::KpEqual:
            return "KpEqual";
        case Keyboard::LeftShift:
            return "LeftShift";
        case Keyboard::LeftControl:
            return "LeftControl";
        case Keyboard::LeftAlt:
            return "LeftAlt";
        case Keyboard::LeftSuper:
            return "LeftSuper";
        case Keyboard::RightShift:
            return "RightShift";
        case Keyboard::RightControl:
            return "RightControl";
        case Keyboard::RightAlt:
            return "RightAlt";
        case Keyboard::RightSuper:
            return "RightSuper";
        case Keyboard::Menu:
            return "Menu";
        default:
            return "Unknown";
    }
}

std::string Button::State::String() const
{
    if (Action == Button::Unknown) {
        return "";
    }

    std::vector<std::string> modifiers;

    if (Modifiers[ModifierKey::Ctrl]) {
        modifiers.push_back("Ctrl");
    }
    if (Modifiers[ModifierKey::Alt]) {
        modifiers.push_back("Alt");
    }
    if (Modifiers[ModifierKey::Shift]) {
        modifiers.push_back("Shift");
    }
    if (Modifiers[ModifierKey::Super]) {
        modifiers.push_back("Super");
    }
    if (Modifiers[ModifierKey::CapsLock]) {
        modifiers.push_back("CapsLock");
    }
    if (Modifiers[ModifierKey::NumLock]) {
        modifiers.push_back("NumLock");
    }

    return fmt::format("{} with {} modifiers", to_string(Action), fmt::join(modifiers, "| "));
}

std::string KeyboardState::String() const
{
    std::string str;

    for (uint32_t i = 0; i < Keyboard::Count; ++i) {
        auto& state = Keys[i];
        if (state.Action != Button::Unknown) {
            str = fmt::format("{}\n {} {}",
                str,
                key_string(static_cast<Keyboard::Key>(i)),
                state.String());
        }
    }
    return str;
}

void Input::KeyCallback(GLFWwindow* aWindow,
    int32_t                         aKey,
    int32_t                         aScancode,
    int32_t                         aAction,
    int32_t                         aMods)
{
    auto*         win   = static_cast<WatoWindow*>(glfwGetWindowUserPointer(aWindow));
    Input&        input = win->GetInput().Latest();
    Keyboard::Key key   = to_key(aKey);

    // don't reset current state, it will discard input and make movement choppy
    input.SetKey(key, to_action(aAction));
    fmt::print("key {} is {}\n", key_string(key), to_string(to_action(aAction)));

    if (aMods & GLFW_MOD_SHIFT) {
        input.SetKeyModifier(key, ModifierKey::Shift);
    }
    if (aMods & GLFW_MOD_CONTROL) {
        input.SetKeyModifier(key, ModifierKey::Ctrl);
    }
    if (aMods & GLFW_MOD_ALT) {
        input.SetKeyModifier(key, ModifierKey::Alt);
    }
    if (aMods & GLFW_MOD_SUPER) {
        input.SetKeyModifier(key, ModifierKey::Super);
    }
    if (aMods & GLFW_MOD_CAPS_LOCK) {
        input.SetKeyModifier(key, ModifierKey::CapsLock);
    }
    if (aMods & GLFW_MOD_NUM_LOCK) {
        input.SetKeyModifier(key, ModifierKey::NumLock);
    }
    fmt::print("input: {}\n", win->GetInput().Latest().KeyboardState.String());

    if (win->GetInput().Previous()) {
        fmt::print("prev input: {}\n", win->GetInput().Previous()->KeyboardState.String());
    }
}

void Input::CursorPosCallback(GLFWwindow* aWindow, double aXpos, double aYpos)
{
    auto*  win   = static_cast<WatoWindow*>(glfwGetWindowUserPointer(aWindow));
    Input& input = win->GetInput().Latest();

    input.SetMousePos(aXpos, aYpos);
}

void Input::MouseButtonCallback(GLFWwindow* aWindow,
    int32_t                                 aButton,
    int32_t                                 aAction,
    int32_t                                 aMods)
{
    auto*         win    = static_cast<WatoWindow*>(glfwGetWindowUserPointer(aWindow));
    Input&        input  = win->GetInput().Latest();
    Mouse::Button button = to_mouse_button(aButton);

    input.SetMouseButtonPressed(button, to_action(aAction));

    if (aMods & GLFW_MOD_SHIFT) {
        input.SetMouseButtonModifier(button, ModifierKey::Shift);
    }
    if (aMods & GLFW_MOD_CONTROL) {
        input.SetMouseButtonModifier(button, ModifierKey::Ctrl);
    }
    if (aMods & GLFW_MOD_ALT) {
        input.SetMouseButtonModifier(button, ModifierKey::Alt);
    }
    if (aMods & GLFW_MOD_SUPER) {
        input.SetMouseButtonModifier(button, ModifierKey::Super);
    }
    if (aMods & GLFW_MOD_CAPS_LOCK) {
        input.SetMouseButtonModifier(button, ModifierKey::CapsLock);
    }
    if (aMods & GLFW_MOD_NUM_LOCK) {
        input.SetMouseButtonModifier(button, ModifierKey::NumLock);
    }
}

void Input::ScrollCallback(GLFWwindow* aWindow, double aXoffset, double aYoffset)
{
    auto*  win   = static_cast<WatoWindow*>(glfwGetWindowUserPointer(aWindow));
    Input& input = win->GetInput().Latest();

    input.SetMouseScroll(aXoffset, aYoffset);
}

void Input::SetMouseButtonPressed(Mouse::Button aButton, Button::Action aAction)
{
    MouseState.Buttons[aButton].Action = aAction;
}

void Input::SetMouseButtonModifier(Mouse::Button aButton, ModifierKey aMod)
{
    MouseState.Buttons[aButton].Modifiers[aMod] = true;
}

void Input::SetMousePos(double aX, double aY)
{
    MouseState.Pos.x = aX;
    MouseState.Pos.y = aY;
}

void Input::SetMouseScroll(double aXoffset, double aYoffset)
{
    MouseState.Scroll.x = aXoffset;
    MouseState.Scroll.y = aYoffset;
}

void Input::SetKey(Keyboard::Key aKey, Button::Action aAction)
{
    KeyboardState.Keys[aKey].Action = aAction;
}

void Input::SetKeyModifier(Keyboard::Key aKey, ModifierKey aMod)
{
    KeyboardState.Keys[aKey].Modifiers[aMod] = true;
}

void Input::DrawImgui(const Camera& aCam,
    const glm::vec3&                aCamPos,
    const float                     aWidth,
    const float                     aHeight) const
{
    auto dir = WorldMousePos(aCam, aCamPos, aWidth, aHeight);

    ImGui::Text("Input Information");
    ImGui::Text("Mouse: %s", glm::to_string(MouseState.Pos).c_str());
    ImGui::Text("Ray cast: %s", glm::to_string(dir).c_str());
}

glm::vec3 Input::WorldMousePos(const Camera& aCam,
    const glm::vec3&                         aCamPos,
    const float                              aWidth,
    const float                              aHeight) const
{
    // viewport -> NDC
    float xNdc = 1.0f - 2.0f * MouseState.Pos.x / aWidth;
    float yNdc = 1.0f - 2.0f * MouseState.Pos.y / aHeight;

    // NDC -> view
    const auto& invProj = glm::inverse(aCam.Projection(aWidth, aHeight));
    glm::vec4   rayNdc(xNdc, yNdc, 1.0f, 1.0f);
    glm::vec4   rayView = rayNdc * invProj;

    // view -> world
    const auto& invView = glm::inverse(aCam.View(aCamPos));
    rayView             = glm::vec4(rayView.x, rayView.y, 1.0f, 0.0f);
    return glm::normalize(rayView * invView);
}
