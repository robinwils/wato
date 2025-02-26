#include "core/action.hpp"

#include <stdexcept>

#include "components/camera.hpp"
#include "components/placement_mode.hpp"
#include "components/scene_object.hpp"
#include "components/transform3d.hpp"
#include "core/ray.hpp"
#include "core/sys.hpp"
#include "entt/signal/dispatcher.hpp"
#include "systems/systems.hpp"

using namespace entt::literals;

void ActionSystem::init_listeners()
{
    auto& dispatcher = m_registry.ctx().get<entt::dispatcher&>();
    dispatcher.sink<CameraMovement>().connect<&ActionSystem::camera_movement>(this);
    dispatcher.sink<BuildTower>().connect<&ActionSystem::build_tower>(this);
    dispatcher.sink<TowerPlacementMode>().connect<&ActionSystem::tower_placement_mode>(this);
}

void ActionSystem::udpate_win_size(int _w, int _h)
{
    m_win_width  = _w;
    m_win_height = _h;
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
        break;
    }
}

void ActionSystem::build_tower(BuildTower bt) {}
void ActionSystem::tower_placement_mode(TowerPlacementMode m)
{
    glm::vec4 ray;
    for (auto&& [entity, cam, t] : m_registry.view<Camera, Transform3D>().each()) {
        ray = ray_cast(cam, m_win_width, m_win_height, m.mousePos);

        break;
    }

    if (m_registry.valid(m_ghost_tower)) {
        if (!m.enable) {
            DBG("valid entity, exiting placement mode");
            m_registry.destroy(m_ghost_tower);
            m_ghost_tower = entt::null;
            return;
        }
        DBG("valid entity, patching ghost tower");
        m_registry.patch<Transform3D>(m_ghost_tower, [ray](auto& t) {
            t.position.x = ray.x;
            t.position.z = ray.y;
        });
    } else if (m.enable) {
        DBG("invalid entity, creating ghost tower");
        m_ghost_tower = m_registry.create();
        m_registry.emplace<SceneObject>(m_ghost_tower, "tower_model"_hs);
        m_registry.emplace<Transform3D>(m_ghost_tower, glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.1f));
        m_registry.emplace<PlacementMode>(m_ghost_tower);
    }
}
