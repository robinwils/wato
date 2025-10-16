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

    for (auto& [_, binding] : mBindings) {
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
                                   binding.Action.AddExtraInputInfo(aInput);
                                   actions.push_back(binding.Action);
                               } else if (
                                   binding.KeyState.State == KeyState::State::PressOnce
                                   && aInput.MouseState.IsKeyPressed(aButton)
                                   && (aInput.PrevMouseState.IsKeyReleased(aButton)
                                       || aInput.PrevMouseState.IsKeyUnknown(aButton))) {
                                   binding.Action.AddExtraInputInfo(aInput);
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
    mBindings.emplace(aActionStr, ActionBinding(aState, aAction));
}
