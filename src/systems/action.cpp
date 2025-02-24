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
            case CameraMovement::CameraForward:
                t.position += speed * cam.front;
                break;
            case CameraMovement::CameraLeft:
                t.position += speed * cam.right();
                break;
            case CameraMovement::CameraBack:
                t.position -= speed * cam.front;
                break;
            case CameraMovement::CameraRight:
                t.position -= speed * cam.right();
                break;
            default:
                throw std::runtime_error("wrong action for camera movement");
        }
    }
}
