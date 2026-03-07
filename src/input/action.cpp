#include "input/action.hpp"

#include <spdlog/spdlog.h>

#include "components/imgui.hpp"
#include "components/placement_mode.hpp"
#include "components/scene_object.hpp"
#include "components/transform3d.hpp"
#include "core/sys/log.hpp"
#include "registry/registry.hpp"

bool KeyState::IsTriggered(const Input& aInput) const
{
    if (const auto* key = std::get_if<Keyboard::Key>(&Key)) {
        if (aInput.UiWantsKeyboard) return false;
        if (State == State::Hold)
            return (aInput.KeyboardState.IsKeyPressed(*key)
                    && aInput.PrevKeyboardState.IsKeyPressed(*key))
                   || aInput.KeyboardState.IsKeyRepeat(*key);
        if (State == State::PressOnce)
            return aInput.KeyboardState.IsKeyPressed(*key)
                   && (aInput.PrevKeyboardState.IsKeyReleased(*key)
                       || aInput.PrevKeyboardState.IsKeyUnknown(*key));
    } else if (const auto* btn = std::get_if<Mouse::Button>(&Key)) {
        if (aInput.UiWantsMouse) return false;
        if (State == State::Hold)
            return (aInput.MouseState.IsKeyPressed(*btn)
                    && aInput.PrevMouseState.IsKeyPressed(*btn))
                   || aInput.MouseState.IsKeyRepeat(*btn);
        if (State == State::PressOnce)
            return aInput.MouseState.IsKeyPressed(*btn)
                   && (aInput.PrevMouseState.IsKeyReleased(*btn)
                       || aInput.PrevMouseState.IsKeyUnknown(*btn));
    }
    return false;
}

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

void ActionBindings::AddBinding(
    const std::string& aActionStr,
    const KeyState&    aState,
    const Action&      aAction)
{
    mBindings.emplace(aActionStr, ActionBinding(aState, aAction));
}

void ActionContextStack::EnterPlacement(Registry& aRegistry, TowerType aTower)
{
    WATO_TRACE(aRegistry, "entering placement mode");
    Stack.push_back({ActionBindings::PlacementDefaults(), PlacementState{.Tower = aTower}});

    auto ghostTower = aRegistry.create();
    WATO_DBG(aRegistry, "created ghost tower {}", ghostTower);
    aRegistry.emplace<SceneObject>(ghostTower, "tower_model"_hs);
    glm::vec3 startPos{0.0f};
    if (auto** ip = aRegistry.ctx().find<const Input*>()) {
        if (const auto& hit = (*ip)->MouseWorldIntersect()) {
            startPos = *hit;
        }
    }
    aRegistry
        .emplace<Transform3D>(ghostTower, startPos, glm::identity<glm::quat>(), glm::vec3(0.1f));
    aRegistry.emplace<PlacementMode>(ghostTower);
    aRegistry.emplace<ImguiDrawable>(ghostTower, "Placement ghost tower", true);
}

void ActionContextStack::ExitPlacement(Registry& aRegistry)
{
    if (!GetState<PlacementState>()) return;

    auto view = aRegistry.view<PlacementMode>();
    for (auto entity : view) {
        aRegistry.destroy(entity);
        WATO_DBG(aRegistry, "destroyed ghost tower {}", entity);
    }
    Stack.pop_back();
    WATO_TRACE(aRegistry, "exited placement mode");
}

void ActionContextStack::TogglePlacement(
    Registry&                    aRegistry,
    const PlacementModePayload& aPayload)
{
    if (GetState<PlacementState>()) {
        ExitPlacement(aRegistry);
    } else {
        EnterPlacement(aRegistry, aPayload.Tower);
    }
}
