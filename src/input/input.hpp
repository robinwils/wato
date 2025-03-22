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
        enum Action Action;
        bool        Modifiers[6];
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
    KeyboardState() : Keys() {}

    Button::State Keys[Keyboard::Key::Count];
};

struct MouseState {
    MouseState() : Pos(), Scroll(), Buttons() {}

    void Clear();

    glm::fvec2    Pos, Scroll;
    Button::State Buttons[3];
};

std::string key_string(const Keyboard::Key& aK);

class Input
{
   public:
    Input() : MouseState(), mTowerPlacementMode(false), mCanBuild(true) {}

    void Init();

    // raw callback handlers
    static void KeyCallback(GLFWwindow* aWindow,
        int32_t                         aKey,
        int32_t                         aScancode,
        int32_t                         aAction,
        int32_t                         aMods);

    static void ScrollCallback(GLFWwindow* aWindow, double aXoffset, double aYoffset);
    static void CursorPosCallback(GLFWwindow* aWindow, double aXpos, double aYpos);
    static void MouseButtonCallback(GLFWwindow* aWindow,
        int32_t                                 aButton,
        int32_t                                 aAction,
        int32_t                                 aMods);

    void SetMouseButtonPressed(Mouse::Button aButton, Button::Action aAction);
    void SetMouseButtonModifier(Mouse::Button aButton, ModifierKey aMod);
    void SetMousePos(double aX, double aY);
    void SetMouseScroll(double aXoffset, double aYoffset);
    void SetKey(Keyboard::Key aKey, Button::Action aState);
    void SetKeyModifier(Keyboard::Key aKey, ModifierKey aMod);

    void ExitTowerPlacementMode() { mTowerPlacementMode = false; }
    void EnterTowerPlacementMode() { mTowerPlacementMode = true; }

    [[nodiscard]] bool IsPlacementMode() const noexcept { return mTowerPlacementMode; }
    [[nodiscard]] bool IsAbleToBuild() const noexcept { return mCanBuild; }

    void SetCanBuild(bool aEnable) noexcept { mCanBuild = aEnable; }

    bool IsMouseButtonPressed(Mouse::Button aButton) const
    {
        return MouseState.Buttons[aButton].Action == Button::Press;
    }
    bool IsKeyPressed(Keyboard::Key aKey) const
    {
        return KeyboardState.Keys[aKey].Action == Button::Press;
    }
    bool IsKeyRepeat(Keyboard::Key aKey) const
    {
        return KeyboardState.Keys[aKey].Action == Button::Repeat;
    }
    bool IsKeyReleased(Keyboard::Key aKey) const
    {
        return KeyboardState.Keys[aKey].Action == Button::Release;
    }
    bool IsKeyUnknown(Keyboard::Key aKey) const
    {
        return KeyboardState.Keys[aKey].Action == Button::Unknown;
    }

    bool IsPrevKeyPressed(Keyboard::Key aKey) const
    {
        return PrevKeyboardState.Keys[aKey].Action == Button::Press;
    }
    bool IsPrevKeyRepeat(Keyboard::Key aKey) const
    {
        return PrevKeyboardState.Keys[aKey].Action == Button::Repeat;
    }
    bool IsPrevKeyReleased(Keyboard::Key aKey) const
    {
        return PrevKeyboardState.Keys[aKey].Action == Button::Release;
    }
    bool IsPrevKeyUnknown(Keyboard::Key aKey) const
    {
        return PrevKeyboardState.Keys[aKey].Action == Button::Unknown;
    }

    void DrawImgui(const Camera& aCamera,
        const glm::vec3&         aCamPos,
        const float              aWidth,
        const float              aHeight) const;

    /**
     * @brief unproject mouse screen coordinates to world view
     *
     * @param cam camera component
     * @param cam_pos camera position
     * @param w screen width
     * @param h screen height
     */
    glm::vec3 WorldMousePos(const Camera& aCam,
        const glm::vec3&                  aCamPos,
        const float                       aWidth,
        const float                       aHeight) const;

    void Clear() { MouseState.Clear(); }

    struct MouseState    MouseState;
    struct KeyboardState KeyboardState, PrevKeyboardState;

   private:
    bool mTowerPlacementMode, mCanBuild;
};
