#include "input/input.hpp"

#include "core/action.hpp"
#include "core/registry.hpp"
#include "entt/signal/dispatcher.hpp"

void processCameraInputs(Input& aInput, entt::dispatcher& aDispatcher, double aTimeDelta)
{
    if (aInput.IsKeyPressed(Keyboard::W) || aInput.IsKeyRepeat(Keyboard::W)) {
        aDispatcher.trigger(
            CameraMovement{.Action = CameraMovement::CameraForward, .Delta = aTimeDelta});
    }
    if (aInput.IsKeyPressed(Keyboard::A) || aInput.IsKeyRepeat(Keyboard::A)) {
        aDispatcher.trigger(
            CameraMovement{.Action = CameraMovement::CameraLeft, .Delta = aTimeDelta});
    }
    if (aInput.IsKeyPressed(Keyboard::S) || aInput.IsKeyRepeat(Keyboard::S)) {
        aDispatcher.trigger(
            CameraMovement{.Action = CameraMovement::CameraBack, .Delta = aTimeDelta});
    }
    if (aInput.IsKeyPressed(Keyboard::D) || aInput.IsKeyRepeat(Keyboard::D)) {
        aDispatcher.trigger(
            CameraMovement{.Action = CameraMovement::CameraRight, .Delta = aTimeDelta});
    }
    if (aInput.MouseState.Scroll.y != 0) {
        aDispatcher.trigger(CameraMovement{.Action = CameraMovement::CameraZoom,
            .Delta = (aInput.MouseState.Scroll.y) * 5.0f * aTimeDelta});
        aInput.MouseState.Scroll.y = 0;
    }
}

void processBuildInputs(Input& aInput, entt::dispatcher& aDispatcher)
{
    if (aInput.IsKeyPressed(Keyboard::B) && !aInput.IsPrevKeyPressed(Keyboard::B)
        && !aInput.IsMouseButtonPressed(Mouse::Left)) {
        if (!aInput.IsPlacementMode()) {
            aInput.EnterTowerPlacementMode();
        }
        aDispatcher.trigger(TowerPlacementMode{true});
    }

    if (aInput.IsPlacementMode()) {
        aDispatcher.trigger(TowerPlacementMode{true});
    }

    if (aInput.IsPlacementMode()) {
        if (aInput.IsKeyPressed(Keyboard::Escape)) {
            aInput.ExitTowerPlacementMode();
            aDispatcher.trigger(TowerPlacementMode{false});
        }

        if (aInput.IsMouseButtonPressed(Mouse::Left)) {
            aDispatcher.trigger(BuildTower{});
        }
    }
}

void processInputs(Registry& aRegistry, double aTimeDelta)
{
    auto& input      = aRegistry.ctx().get<Input&>();
    auto& dispatcher = aRegistry.ctx().get<entt::dispatcher&>();

    processCameraInputs(input, dispatcher, aTimeDelta);
    processBuildInputs(input, dispatcher);
}
