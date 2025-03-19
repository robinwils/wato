#pragma once

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <string>

#include "components/camera.hpp"

struct Keyboard {
    enum Key {
        Space,
        Apostrophe /* ' */,
        Comma /* , */,
        Minus /* - */,
        Period /* . */,
        Slash /* / */,
        Num0,
        Num1,
        Num2,
        Num3,
        Num4,
        Num5,
        Num6,
        Num7,
        Num8,
        Num9,
        Semicolon /* ; */,
        Equal /* = */,
        A,
        B,
        C,
        D,
        E,
        F,
        G,
        H,
        I,
        J,
        K,
        L,
        M,
        N,
        O,
        P,
        Q,
        R,
        S,
        T,
        U,
        V,
        W,
        X,
        Y,
        Z,
        LeftBracket /* [ */,
        Backslash /* \ */,
        RightBracket /* ] */,
        GraveAccent /* ` */,
        World1 /* non-US #1 */,
        World2 /* non-US #2 */,

        /* Function keys */
        Escape,
        Enter,
        Tab,
        Backspace,
        Insert,
        Delete,
        Right,
        Left,
        Down,
        Up,
        PageUp,
        PageDown,
        Home,
        End,
        CapsLock,
        ScrollLock,
        NumLock,
        PrintScreen,
        Pause,
        F1,
        F2,
        F3,
        F4,
        F5,
        F6,
        F7,
        F8,
        F9,
        F10,
        F11,
        F12,
        F13,
        F14,
        F15,
        F16,
        F17,
        F18,
        F19,
        F20,
        F21,
        F22,
        F23,
        F24,
        F25,
        Kp0,
        Kp1,
        Kp2,
        Kp3,
        Kp4,
        Kp5,
        Kp6,
        Kp7,
        Kp8,
        Kp9,
        KpDecimal,
        KpDivide,
        KpMultiply,
        KpSubtract,
        KpAdd,
        KpEnter,
        KpEqual,
        LeftShift,
        LeftControl,
        LeftAlt,
        LeftSuper,
        RightShift,
        RightControl,
        RightAlt,
        RightSuper,
        Menu,
        Count,
    };
};

struct Mouse {
    enum Button {
        Unknown,
        Left,
        Right,
        Middle,
    };
};

struct Button {
    enum Action {
        Unknown,
        Press,
        Release,
        Repeat,
    };

    struct State {
        Action action;
        bool   modifiers[6];
    };
};

enum ModifierKey {
    None,
    Shift,
    Ctrl,
    Alt,
    Super,
    CapsLock,
    NumLock,
};

struct KeyboardState {
    KeyboardState() : keys() {}

    Button::State keys[Keyboard::Key::Count];
};

struct MouseState {
    MouseState() : pos(), scroll(), buttons() {}

    void clear();

    glm::dvec2    pos, scroll;
    Button::State buttons[3];
};

std::string key_string(const Keyboard::Key& k);

class Input
{
   public:
    Input(GLFWwindow* _window);

    void init();
    void setMouseButtonPressed(Mouse::Button _button, Button::Action _action);
    void setMouseButtonModifier(Mouse::Button _button, ModifierKey _mod);
    void setMousePos(double _x, double _y);
    void setMouseScroll(double _xoffset, double _yoffset);
    void setKey(Keyboard::Key _key, Button::Action _state);
    void setKeyModifier(Keyboard::Key _key, ModifierKey _mod);

    void exitTowerPlacementMode() { m_tower_placement_mode = false; }

    bool isMouseButtonPressed(Mouse::Button _button) const
    {
        return mouseState.buttons[_button].action == Button::Press;
    }
    bool isKeyPressed(Keyboard::Key _key) const
    {
        return keyboardState.keys[_key].action == Button::Press;
    }
    bool isKeyRepeat(Keyboard::Key _key) const
    {
        return keyboardState.keys[_key].action == Button::Repeat;
    }
    bool isKeyReleased(Keyboard::Key _key) const
    {
        return keyboardState.keys[_key].action == Button::Release;
    }
    bool isKeyUnknown(Keyboard::Key _key) const
    {
        return keyboardState.keys[_key].action == Button::Unknown;
    }

    bool isPrevKeyPressed(Keyboard::Key _key) const
    {
        return prevKeyboardState.keys[_key].action == Button::Press;
    }
    bool isPrevKeyRepeat(Keyboard::Key _key) const
    {
        return prevKeyboardState.keys[_key].action == Button::Repeat;
    }
    bool isPrevKeyReleased(Keyboard::Key _key) const
    {
        return prevKeyboardState.keys[_key].action == Button::Release;
    }
    bool isPrevKeyUnknown(Keyboard::Key _key) const
    {
        return prevKeyboardState.keys[_key].action == Button::Unknown;
    }

    void drawImgui(const Camera& cam, const glm::vec3& cam_pos, const float w, const float h) const;

    /**
     * @brief unproject mouse screen coordinates to world view
     *
     * @param cam camera component
     * @param cam_pos camera position
     * @param w screen width
     * @param h screen height
     */
    glm::vec3 worldMousePos(const Camera& cam,
        const glm::vec3&                  cam_pos,
        const float                       w,
        const float                       h) const;

    void clear() { mouseState.clear(); }

    MouseState    mouseState;
    KeyboardState keyboardState, prevKeyboardState;
    bool          m_tower_placement_mode;

   private:
    GLFWwindow* mWindow;
};

static void cursorPosCb(GLFWwindow* _window, double _xpos, double _ypos);
