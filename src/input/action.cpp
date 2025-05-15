#include "input/action.hpp"

#include <spdlog/spdlog.h>

#include "components/tower.hpp"

ActionBindings ActionBindings::Defaults()
{
    ActionBindings bindings;
    bindings.AddBinding(
        "move_left",
        KeyState{
            .Key       = Keyboard::A,
            .State     = KeyState::State::Hold,
            .Modifiers = 0,
        },
        kMoveLeftAction);
    bindings.AddBinding(
        "move_right",
        KeyState{
            .Key       = Keyboard::D,
            .State     = KeyState::State::Hold,
            .Modifiers = 0,
        },
        kMoveRightAction);
    bindings.AddBinding(
        "move_front",
        KeyState{
            .Key       = Keyboard::W,
            .State     = KeyState::State::Hold,
            .Modifiers = 0,
        },
        kMoveFrontAction);
    bindings.AddBinding(
        "move_back",
        KeyState{
            .Key       = Keyboard::S,
            .State     = KeyState::State::Hold,
            .Modifiers = 0,
        },
        kMoveBackAction);
    bindings.AddBinding(
        "enter_placement_ctx",
        KeyState{
            .Key       = Keyboard::B,
            .State     = KeyState::State::PressOnce,
            .Modifiers = 0,
        },
        kEnterPlacementModeAction);
    bindings.AddBinding(
        "send_creep",
        KeyState{
            .Key       = Keyboard::C,
            .State     = KeyState::State::PressOnce,
            .Modifiers = 0,
        },
        kSendCreepAction);
    return bindings;
}

ActionBindings ActionBindings::PlacementDefaults()
{
    ActionBindings bindings = Defaults();
    bindings.AddBinding(
        "build_tower",
        KeyState{
            .Key       = Mouse::Button::Left,
            .State     = KeyState::State::PressOnce,
            .Modifiers = 0,
        },
        kBuildTowerAction);
    bindings.AddBinding(
        "exit_placement_mode",
        KeyState{
            .Key       = Keyboard::Escape,
            .State     = KeyState::State::PressOnce,
            .Modifiers = 0,
        },
        kExitPlacementModeAction);
    return bindings;
}

ActionBindings::actions_type ActionBindings::ActionsFromInput(const Input& aInput)
{
    ActionBindings::actions_type actions;

    for (const auto& [_, binding] : mBindings) {
        std::visit(
            VariantVisitor{// handle keyboard binding
                           [&](const Keyboard::Key& aKey) {
                               if (binding.KeyState.State == KeyState::State::Hold
                                   && ((aInput.KeyboardState.IsKeyPressed(aKey)
                                        && aInput.PrevKeyboardState.IsKeyPressed(aKey))
                                       || aInput.KeyboardState.IsKeyRepeat(aKey))) {
                                   actions.push_back(binding.Action);
                               } else if (
                                   binding.KeyState.State == KeyState::State::PressOnce
                                   && aInput.KeyboardState.IsKeyPressed(aKey)
                                   && (aInput.PrevKeyboardState.IsKeyReleased(aKey)
                                       || aInput.PrevKeyboardState.IsKeyUnknown(aKey))) {
                                   actions.push_back(binding.Action);
                               }
                           },
                           [&](const Mouse::Button& aButton) {
                               if (binding.KeyState.State == KeyState::State::Hold
                                   && ((aInput.MouseState.IsKeyPressed(aButton)
                                        && aInput.PrevMouseState.IsKeyPressed(aButton))
                                       || aInput.MouseState.IsKeyRepeat(aButton))) {
                                   actions.push_back(binding.Action);
                               } else if (
                                   binding.KeyState.State == KeyState::State::PressOnce
                                   && aInput.MouseState.IsKeyPressed(aButton)
                                   && (aInput.PrevMouseState.IsKeyReleased(aButton)
                                       || aInput.PrevMouseState.IsKeyUnknown(aButton))) {
                                   actions.push_back(binding.Action);
                               }
                           }},
            binding.KeyState.Key);
    }

    if (aInput.MouseState.Scroll.y > 0) {
        actions.push_back(kMoveDownAction);
    } else if (aInput.MouseState.Scroll.y < 0) {
        actions.push_back(kMoveUpAction);
    }

    return actions;
}

void ActionBindings::AddBinding(
    const std::string& aActionStr,
    const KeyState&    aState,
    const Action&      aAction)
{
    mBindings[aActionStr] = ActionBinding{
        .KeyState = aState,
        .Action   = aAction,
    };
}
