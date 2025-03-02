#include "core/action.hpp"

#include <stdexcept>

#include "bx/bx.h"
#include "components/camera.hpp"
#include "components/placement_mode.hpp"
#include "components/scene_object.hpp"
#include "components/tile.hpp"
#include "components/transform3d.hpp"
#include "core/cache.hpp"
#include "core/ray.hpp"
#include "core/sys.hpp"
#include "entt/signal/dispatcher.hpp"
#include "glm/geometric.hpp"
#include "glm/gtx/string_cast.hpp"
#include "renderer/plane_primitive.hpp"
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
    glm::vec3 intersect;
    glm::vec3 cam_pos;
    for (auto&& [entity, cam, tcam] : m_registry.view<Camera, Transform3D>().each()) {
        for (auto&& [entity, t, obj] : m_registry.view<Transform3D, SceneObject, Tile>().each()) {
            cam_pos  = tcam.position;
            auto ray = Ray(cam_pos, m.mousePos);

            const auto& primitives = MODEL_CACHE[obj.model_hash];
            BX_ASSERT(primitives->size() == 1, "plane should have 1 primitive");
            const auto* plane = static_cast<PlanePrimitive*>(primitives->back());

            intersect = ray.intersect_plane(cam, m_win_width, m_win_height, plane->normal(t.rotation));
            break;
        }
        break;
    }

    if (m_registry.valid(m_ghost_tower)) {
        if (!m.enable) {
            DBG("valid entity, exiting placement mode");
            m_registry.destroy(m_ghost_tower);
            m_ghost_tower = entt::null;
            return;
        }

        DBG("valid entity, patching ghost tower, cam = %s", glm::to_string(intersect).c_str());
        m_registry.patch<Transform3D>(m_ghost_tower, [intersect](auto& t) {
            t.position.x = intersect.x;
            t.position.z = intersect.z;
        });
    } else if (m.enable) {
        DBG("invalid entity, creating ghost tower");
        m_ghost_tower = m_registry.create();
        m_registry.emplace<SceneObject>(m_ghost_tower, "tower_model"_hs);
        m_registry.emplace<Transform3D>(m_ghost_tower, glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.1f));
        m_registry.emplace<PlacementMode>(m_ghost_tower);
    }
}
