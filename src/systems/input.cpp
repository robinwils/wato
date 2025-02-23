#include "input/input.hpp"

#include "core/action.hpp"
#include "core/registry.hpp"
#include "entt/signal/dispatcher.hpp"

void processInputs(Registry& registry, double time_delta)
{
    const auto& input      = registry.ctx().get<Input&>();
    auto&       dispatcher = registry.ctx().get<entt::dispatcher&>();

    if (input.isKeyPressed(Keyboard::Key::W) || input.isKeyRepeat(Keyboard::Key::W)) {
        dispatcher.trigger(CameraMovement{CameraForward, time_delta});
    }
    if (input.isKeyPressed(Keyboard::Key::A) || input.isKeyRepeat(Keyboard::Key::A)) {
        dispatcher.trigger(CameraMovement{CameraLeft, time_delta});
    }
    if (input.isKeyPressed(Keyboard::Key::S) || input.isKeyRepeat(Keyboard::Key::S)) {
        dispatcher.trigger(CameraMovement{CameraBack, time_delta});
    }
    if (input.isKeyPressed(Keyboard::Key::D) || input.isKeyRepeat(Keyboard::Key::D)) {
        dispatcher.trigger(CameraMovement{CameraRight, time_delta});
    }
}
