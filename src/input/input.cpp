#include "input.hpp"

#include <imgui.h>
#include <spdlog/fmt/bundled/ranges.h>

#include <cstring>
#include <glm/gtx/string_cast.hpp>
#include <vector>

#include "components/camera.hpp"
#include "core/queue/ring_buffer.hpp"
#include "core/sys/log.hpp"
#include "core/window.hpp"

std::string MouseState::String() const
{
    std::string str;

    for (uint32_t i = 0; i < MouseState::kCapacity; ++i) {
        auto& state = Inputs[i];
        if (state.Action != Button::Unknown) {
            str = fmt::format(
                "{}\n {} {}",
                str,
                mouse_button_string(static_cast<Mouse::Button>(i)),
                state.String());
        }
    }
    return str;
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

std::string mouse_button_string(const Mouse::Button& aButton)
{
    switch (aButton) {
        case Mouse::Button::Left:
            return "Left";
        case Mouse::Button::Right:
            return "Right";
        case Mouse::Button::Middle:
            return "Middle";
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

    if (IsModifierSet(Modifiers, ModifierKey::Ctrl)) {
        modifiers.push_back("Ctrl");
    }
    if (IsModifierSet(Modifiers, ModifierKey::Alt)) {
        modifiers.push_back("Alt");
    }
    if (IsModifierSet(Modifiers, ModifierKey::Shift)) {
        modifiers.push_back("Shift");
    }
    if (IsModifierSet(Modifiers, ModifierKey::Super)) {
        modifiers.push_back("Super");
    }
    if (IsModifierSet(Modifiers, ModifierKey::CapsLock)) {
        modifiers.push_back("CapsLock");
    }
    if (IsModifierSet(Modifiers, ModifierKey::NumLock)) {
        modifiers.push_back("NumLock");
    }

    return fmt::format("{} with {} modifiers", to_string(Action), fmt::join(modifiers, "| "));
}

std::string KeyboardState::String() const
{
    std::string str;

    for (uint32_t i = 0; i < Keyboard::Count; ++i) {
        auto& state = Inputs[i];
        if (state.Action != Button::Unknown) {
            str = fmt::format(
                "{}\n {} {}",
                str,
                key_string(static_cast<Keyboard::Key>(i)),
                state.String());
        }
    }
    return str;
}

void Input::DrawImgui(const Camera& aCam, const glm::vec3& aCamPos, WatoWindow& aWin)
{
    const auto& [origin, end] = aWin.MouseUnproject(aCam, aCamPos);

    ImGui::Text("Input Information");
    ImGui::Text("Mouse: %s", glm::to_string(MouseState.Pos).c_str());
    ImGui::Text("Mouse Ray: %s", glm::to_string(end - origin).c_str());
}
