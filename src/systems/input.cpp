#include "input/input.hpp"

#include "core/action.hpp"
#include "core/registry.hpp"
#include "entt/signal/dispatcher.hpp"

void processCameraInputs(Input& input, entt::dispatcher& dispatcher, const double time_delta)
{
    if (input.isKeyPressed(Keyboard::W) || input.isKeyRepeat(Keyboard::W)) {
        dispatcher.trigger(CameraMovement{CameraMovement::CameraForward, time_delta});
    }
    if (input.isKeyPressed(Keyboard::A) || input.isKeyRepeat(Keyboard::A)) {
        dispatcher.trigger(CameraMovement{CameraMovement::CameraLeft, time_delta});
    }
    if (input.isKeyPressed(Keyboard::S) || input.isKeyRepeat(Keyboard::S)) {
        dispatcher.trigger(CameraMovement{CameraMovement::CameraBack, time_delta});
    }
    if (input.isKeyPressed(Keyboard::D) || input.isKeyRepeat(Keyboard::D)) {
        dispatcher.trigger(CameraMovement{CameraMovement::CameraRight, time_delta});
    }
}

void processBuildInputs(Input& input, entt::dispatcher& dispatcher)
{
    if (input.isKeyPressed(Keyboard::B) && !input.isPrevKeyPressed(Keyboard::B)) {
        if (!input.m_tower_placement_mode) input.m_tower_placement_mode = true;
        dispatcher.trigger(TowerPlacementMode{true});
    }

    if (input.m_tower_placement_mode) {
        dispatcher.trigger(TowerPlacementMode{true});
    }

    if (input.m_tower_placement_mode) {
        if (input.isKeyPressed(Keyboard::Escape)) {
            input.m_tower_placement_mode = false;
            dispatcher.trigger(TowerPlacementMode{false});
        }

        if (input.isMouseButtonPressed(Mouse::Left)) {
            input.m_tower_placement_mode = false;
            dispatcher.trigger(BuildTower{});
        }
    }
}

void processInputs(Registry& registry, const double time_delta)
{
    auto& input      = registry.ctx().get<Input&>();
    auto& dispatcher = registry.ctx().get<entt::dispatcher&>();

    // Camera
    // Preview
    processCameraInputs(input, dispatcher, time_delta);
    processBuildInputs(input, dispatcher);
}
