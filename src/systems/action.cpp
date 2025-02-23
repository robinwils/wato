#include "core/action.hpp"

#include <stdexcept>

#include "components/camera.hpp"
#include "components/transform3d.hpp"
#include "entt/signal/dispatcher.hpp"
#include "systems/systems.hpp"

void ActionSystem::init_listeners()
{
    auto& dispatcher = m_registry.ctx().get<entt::dispatcher&>();
    dispatcher.sink<CameraMovement>().connect<&ActionSystem::camera_movement>(this);
}

void ActionSystem::camera_movement(CameraMovement _cm)
{
    for (auto&& [entity, cam, t] : m_registry.view<Camera, Transform3D>().each()) {
        float speed = cam.speed * _cm.time_delta;
        switch (_cm.action) {
            case CameraForward:
                t.position += speed * cam.front;
                break;
            case CameraLeft:
                t.position += speed * cam.right();
                break;
            case CameraBack:
                t.position -= speed * cam.front;
                break;
            case CameraRight:
                t.position -= speed * cam.right();
                break;
            default:
                throw std::runtime_error("wrong action for camera movement");
        }
    }
}
