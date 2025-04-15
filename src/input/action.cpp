#include "input/action.hpp"

ActionBindings ActionBindings::Defaults()
{
    ActionBindings bindings;
    bindings.AddBinding("move_left",
        KeyState{
            .Key       = Keyboard::A,
            .State     = KeyState::State::Hold,
            .Modifiers = 0,
        },
        kMoveLeftAction);
    bindings.AddBinding("move_right",
        KeyState{
            .Key       = Keyboard::D,
            .State     = KeyState::State::Hold,
            .Modifiers = 0,
        },
        kMoveRightAction);
    bindings.AddBinding("move_front",
        KeyState{
            .Key       = Keyboard::W,
            .State     = KeyState::State::Hold,
            .Modifiers = 0,
        },
        kMoveFrontAction);
    bindings.AddBinding("move_back",
        KeyState{
            .Key       = Keyboard::S,
            .State     = KeyState::State::Hold,
            .Modifiers = 0,
        },
        kMoveBackAction);
    return bindings;
}

ActionBindings ActionBindings::PlacementDefaults()
{
    ActionBindings bindings;
    bindings.AddBinding("build_tower",
        KeyState{
            .Key       = Keyboard::B,
            .State     = KeyState::State::PressOnce,
            .Modifiers = 0,
        },
        kBuildTowerAction);
    bindings.AddBinding("pop_context",
        KeyState{
            .Key       = Keyboard::Escape,
            .State     = KeyState::State::PressOnce,
            .Modifiers = 0,
        },
        kBuildTowerAction);
    return bindings;
}

ActionBindings::actions_type ActionBindings::ActionsFromInput(const Input& aInput)
{
    ActionBindings::actions_type actions;

    for (const auto& [_, binding] : mBindings) {
        std::visit(InputButtonVisitor{// handle keyboard binding
                       [&](const Keyboard::Key& aKey) {
                           if (binding.KeyState.State == KeyState::State::Hold
                               && (aInput.KeyboardState.IsKeyPressed(aKey)
                                   || aInput.KeyboardState.IsKeyRepeat(aKey))) {
                               actions.push_back(binding.Action);
                           } else if (binding.KeyState.State == KeyState::State::PressOnce
                                      && aInput.PrevKeyboardState.IsKeyPressed(aKey)
                                      && aInput.PrevKeyboardState.IsKeyReleased(aKey)) {
                               actions.push_back(binding.Action);
                           }
                       },
                       [&](const Mouse::Button& aButton) {}},
            binding.KeyState.Key);
    }

    return actions;
}

void ActionBindings::AddBinding(const std::string& aActionStr,
    const KeyState&                                aState,
    const Action&                                  aAction)
{
    mBindings[aActionStr] = ActionBinding{
        .KeyState = aState,
        .Action   = aAction,
    };
}
