#include "core/action.hpp"

#include <stdexcept>

#include "bx/bx.h"
#include "components/camera.hpp"
#include "components/health.hpp"
#include "components/imgui.hpp"
#include "components/placement_mode.hpp"
#include "components/rigid_body.hpp"
#include "components/scene_object.hpp"
#include "components/tile.hpp"
#include "components/transform3d.hpp"
#include "core/cache.hpp"
#include "core/ray.hpp"
#include "core/sys.hpp"
#include "entt/signal/dispatcher.hpp"
#include "glm/geometric.hpp"
#include "glm/gtx/string_cast.hpp"
#include "input/input.hpp"
#include "reactphysics3d/engine/PhysicsWorld.h"
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
        float speed = cam.speed * _cm.delta;
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
            case CameraMovement::CameraZoom:
                if ((t.position.y >= 1.0f && speed > 0.0f)
                    || (t.position.y <= 10.0f && speed < 0.0f)) {
                    t.position -= speed * cam.up;
                }
                break;
            default:
                throw std::runtime_error("wrong action for camera movement");
        }
        break;
    }
}

glm::vec3 ActionSystem::get_mouse_ray() const
{
    const auto& input = m_registry.ctx().get<Input&>();
    glm::vec3   intersect;

    for (auto&& [entity, cam, tcam] : m_registry.view<Camera, Transform3D>().each()) {
        for (auto&& [entity, t, obj] : m_registry.view<Transform3D, SceneObject, Tile>().each()) {
            auto ray = Ray(tcam.position,
                input.worldMousePos(cam, tcam.position, m_win_width, m_win_height));

            const auto& primitives = MODEL_CACHE[obj.model_hash];
            BX_ASSERT(primitives->size() == 1, "plane should have 1 primitive");
            const auto* plane = static_cast<PlanePrimitive*>(primitives->back());

            float d = ray.intersect_plane(plane->normal(t.rotation));
            return ray.orig + d * ray.dir;
        }
    }
    throw std::runtime_error("should not be here, no terrain or camera was instanced");
}

void ActionSystem::build_tower(BuildTower bt)
{
    auto tower = m_ghost_tower;

    BX_ASSERT(m_registry.valid(tower), "ghost tower must be valid");
    const auto& t          = m_registry.get<Transform3D>(tower);
    auto*       phy_world  = m_registry.ctx().get<rp3d::PhysicsWorld*>();
    auto&       phy_common = m_registry.ctx().get<rp3d::PhysicsCommon>();
    auto*       rb         = phy_world->createRigidBody(t.to_rp3d());
    auto*       box        = phy_common.createBoxShape(rp3d::Vector3(0.5f, 1.0f, 0.5f));

    rb->setType(rp3d::BodyType::STATIC);
    rb->addCollider(box, rp3d::Transform::identity());

    m_registry.emplace<RigidBody>(tower, rb);
    m_registry.emplace<Health>(tower, 100.0f);
    m_registry.remove<PlacementMode>(tower);
    m_registry.remove<ImguiDrawable>(tower);

#if WATO_DEBUG
    rb->setIsDebugEnabled(true);
#endif
    m_ghost_tower = entt::null;
}

void ActionSystem::tower_placement_mode(TowerPlacementMode m)
{
    auto intersect = get_mouse_ray();

    if (m_registry.valid(m_ghost_tower)) {
        if (!m.enable) {
            m_registry.destroy(m_ghost_tower);
            m_ghost_tower = entt::null;
            return;
        }

        m_registry.patch<Transform3D>(m_ghost_tower, [intersect](auto& t) {
            t.position.x = intersect.x;
            t.position.z = intersect.z;
        });
    } else if (m.enable) {
        m_ghost_tower = m_registry.create();
        m_registry.emplace<SceneObject>(m_ghost_tower, "tower_model"_hs);
        m_registry.emplace<Transform3D>(m_ghost_tower,
            glm::vec3(intersect.x, 0.0f, intersect.z),
            glm::vec3(0.0f),
            glm::vec3(0.1f));
        m_registry.emplace<PlacementMode>(m_ghost_tower);
        m_registry.emplace<ImguiDrawable>(m_ghost_tower, "Ghost Tower");
    }
}
