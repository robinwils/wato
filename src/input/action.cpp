#include "input/action.hpp"

#include <spdlog/spdlog.h>

ActionBindings ActionBindings::Defaults()
{
    ActionBindings bindings;
    bindings.AddBinding(
        "move_left",
        KeyState(Keyboard::A, KeyState::State::Hold, 0),
        kMoveLeftAction);
    bindings.AddBinding(
        "move_right",
        KeyState(Keyboard::D, KeyState::State::Hold, 0),
        kMoveRightAction);
    bindings.AddBinding(
        "move_front",
        KeyState(Keyboard::W, KeyState::State::Hold, 0),
        kMoveFrontAction);
    bindings.AddBinding(
        "move_back",
        KeyState(Keyboard::S, KeyState::State::Hold, 0),
        kMoveBackAction);
    bindings.AddBinding(
        "enter_placement_ctx",
        KeyState(Keyboard::B, KeyState::State::PressOnce, 0),
        kEnterPlacementModeAction);
    bindings.AddBinding(
        "send_creep",
        KeyState(Keyboard::C, KeyState::State::PressOnce, 0),
        kSendCreepAction);
    return bindings;
}

ActionBindings ActionBindings::PlacementDefaults()
{
    ActionBindings bindings = Defaults();
    bindings.AddBinding(
        "build_tower",
        KeyState(Mouse::Button::Left, KeyState::State::PressOnce, 0),
        kBuildTowerAction);
    bindings.AddBinding(
        "exit_placement_mode",
        KeyState(Keyboard::Escape, KeyState::State::PressOnce, 0),
        kExitPlacementModeAction);
    return bindings;
}

ActionsType ActionBindings::ActionsFromInput(const Input& aInput)
{
    ActionsType actions;

    struct InputBindingVisitor {
        const Input*   In;
        ActionBinding* Binding;
        ActionsType*   Actions;

        void operator()(const Keyboard::Key& aKey) const
        {
            if (In->UiWantsKeyboard) {
                return;
            }
            if (Binding->KeyState.State == KeyState::State::Hold
                && ((In->KeyboardState.IsKeyPressed(aKey)
                     && In->PrevKeyboardState.IsKeyPressed(aKey))
                    || In->KeyboardState.IsKeyRepeat(aKey))) {
                Actions->push_back(Binding->Action);
            } else if (
                Binding->KeyState.State == KeyState::State::PressOnce
                && In->KeyboardState.IsKeyPressed(aKey)
                && (In->PrevKeyboardState.IsKeyReleased(aKey)
                    || In->PrevKeyboardState.IsKeyUnknown(aKey))) {
                Actions->push_back(Binding->Action);
            }
        }

        void operator()(const Mouse::Button& aButton) const
        {
            if (In->UiWantsMouse) {
                return;
            }
            if (Binding->KeyState.State == KeyState::State::Hold
                && ((In->MouseState.IsKeyPressed(aButton)
                     && In->PrevMouseState.IsKeyPressed(aButton))
                    || In->MouseState.IsKeyRepeat(aButton))) {
                Binding->Action.AddExtraInputInfo(*In);
                Actions->push_back(Binding->Action);
            } else if (
                Binding->KeyState.State == KeyState::State::PressOnce
                && In->MouseState.IsKeyPressed(aButton)
                && (In->PrevMouseState.IsKeyReleased(aButton)
                    || In->PrevMouseState.IsKeyUnknown(aButton))) {
                Binding->Action.AddExtraInputInfo(*In);
                Actions->push_back(Binding->Action);
            }
        }
    };

    for (auto& [_, binding] : mBindings) {
        std::visit(InputBindingVisitor{&aInput, &binding, &actions}, binding.KeyState.Key);
    }

    if (!aInput.UiWantsMouse) {
        if (aInput.MouseState.Scroll.y > 0) {
            actions.push_back(kMoveDownAction);
        } else if (aInput.MouseState.Scroll.y < 0) {
            actions.push_back(kMoveUpAction);
        }
    }

    return actions;
}

void ActionBindings::AddBinding(
    const std::string& aActionStr,
    const KeyState&    aState,
    const Action&      aAction)
{
    mBindings.emplace(aActionStr, ActionBinding(aState, aAction));
}
