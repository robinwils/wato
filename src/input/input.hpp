#pragma once

#include <GLFW/glfw3.h>

#include <array>
#include <cstring>
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
        std::string String() const;
        enum Action Action;
        uint8_t     Modifiers;
    };
};

enum class ModifierKey {
    Shift = 0,
    Ctrl,
    Alt,
    Super,
    CapsLock,
    NumLock,
};

constexpr inline uint8_t ModifierMask(const ModifierKey& aMod)
{
    return 1 << static_cast<uint8_t>(aMod);
}

inline void SetModifier(uint8_t& aMods, const uint8_t& aMod) { aMods |= aMod; }
inline void SetModifier(uint8_t& aMods, const ModifierKey& aMod) { aMods |= ModifierMask(aMod); }
inline void ClearModifier(uint8_t& aMods, const uint8_t& aMod) { aMods &= ~aMod; }
inline void ClearModifier(uint8_t& aMods, const ModifierKey& aMod) { aMods &= ~ModifierMask(aMod); }
inline bool IsModifierSet(const uint8_t& aMods, const uint8_t& aMod) { return (aMods & aMod) != 0; }
inline bool IsModifierSet(const uint8_t& aMods, const ModifierKey& aMod)
{
    return (aMods & ModifierMask(aMod)) != 0;
}

constexpr uint8_t kAltShiftMod = ModifierMask(ModifierKey::Alt) | ModifierMask(ModifierKey::Shift);
constexpr uint8_t kCtrlAltMod  = ModifierMask(ModifierKey::Ctrl) | ModifierMask(ModifierKey::Alt);
constexpr uint8_t kCtrlShiftMod =
    ModifierMask(ModifierKey::Ctrl) | ModifierMask(ModifierKey::Shift);
constexpr uint8_t kCtrlAltShiftMod = ModifierMask(ModifierKey::Ctrl)
                                     | ModifierMask(ModifierKey::Alt)
                                     | ModifierMask(ModifierKey::Shift);

template <std::size_t Size>
struct InputState {
    static constexpr std::size_t kCapacity = Size;
    virtual ~InputState()                  = default;
    virtual void Clear() { std::memset(Inputs.data(), 0, kCapacity * sizeof(Button::State)); }

    [[nodiscard]] bool IsKeyPressed(std::size_t aKey) const
    {
        return Inputs[aKey].Action == Button::Press;
    }
    [[nodiscard]] bool IsKeyRepeat(std::size_t aKey) const
    {
        return Inputs[aKey].Action == Button::Repeat;
    }
    [[nodiscard]] bool IsKeyReleased(std::size_t aKey) const
    {
        return Inputs[aKey].Action == Button::Release;
    }
    [[nodiscard]] bool IsKeyUnknown(std::size_t aKey) const
    {
        return Inputs[aKey].Action == Button::Unknown;
    }
    void SetKey(std::size_t aKey, Button::Action aAction) { Inputs[aKey].Action = aAction; }
    void SetKeyModifier(std::size_t aKey, ModifierKey aMod)
    {
        SetModifier(Inputs[aKey].Modifiers, aMod);
    }

    std::array<Button::State, kCapacity> Inputs;
};

struct KeyboardState : public InputState<Keyboard::Count> {
    std::string String() const;
};

static constexpr uint32_t kNumButtons = 3;
struct MouseState : public InputState<kNumButtons> {
    MouseState() : Pos(), Scroll() {}

    std::string String() const;
    void        Clear() override;
    glm::fvec2  Pos, Scroll;
};

std::string key_string(const Keyboard::Key& aK);
std::string mouse_button_string(const Mouse::Button& aButton);

// TODO: rework Input in different steps (see if doable):
// - This class for current and previous raw input (as before...)
// - detect actions and derive action components (dedicated entity?) inside an input system
//   - double check if Single component struct for input or dedicated action components
// - dedicated separated systems for each if dedicated action components ?
// - only sync actions to server
class Input
{
   public:
    using mouse_state    = InputState<3>;
    using keyboard_state = InputState<Keyboard::Count>;
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

    void ExitTowerPlacementMode() { mTowerPlacementMode = false; }
    void EnterTowerPlacementMode() { mTowerPlacementMode = true; }

    [[nodiscard]] bool IsPlacementMode() const noexcept { return mTowerPlacementMode; }
    [[nodiscard]] bool IsAbleToBuild() const noexcept { return mCanBuild; }

    void SetCanBuild(bool aEnable) noexcept { mCanBuild = aEnable; }

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

    struct MouseState    MouseState, PrevMouseState;
    struct KeyboardState KeyboardState, PrevKeyboardState;

   private:
    bool mTowerPlacementMode, mCanBuild;
};
