#include "core/action.hpp"

#include <functional>
#include <stdexcept>

#include "bx/bx.h"
#include "components/camera.hpp"
#include "components/health.hpp"
#include "components/imgui.hpp"
#include "components/physics.hpp"
#include "components/placement_mode.hpp"
#include "components/rigid_body.hpp"
#include "components/scene_object.hpp"
#include "components/tile.hpp"
#include "components/transform3d.hpp"
#include "core/cache.hpp"
#include "core/ray.hpp"
#include "core/sys.hpp"
#include "entt/entity/fwd.hpp"
#include "entt/signal/dispatcher.hpp"
#include "glm/geometric.hpp"
#include "glm/gtx/string_cast.hpp"
#include "input/input.hpp"
#include "reactphysics3d/engine/PhysicsWorld.h"
#include "renderer/plane_primitive.hpp"
#include "systems/systems.hpp"

using namespace entt::literals;

void ActionSystem::init_listeners(Input& input)
{
    auto& dispatcher = m_registry.ctx().get<entt::dispatcher&>();
    dispatcher.sink<CameraMovement>().connect<&ActionSystem::camera_movement>(this);
    dispatcher.sink<BuildTower>().connect<&ActionSystem::build_tower>(this);
    dispatcher.sink<TowerPlacementMode>().connect<&ActionSystem::tower_placement_mode>(this);
    dispatcher.sink<TowerCreated>().connect<&Input::exitTowerPlacementMode>(input);
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
    if (!m_can_build) return;
    auto tower = m_ghost_tower;

    BX_ASSERT(m_registry.valid(tower), "ghost tower must be valid");
    auto& phy = m_registry.ctx().get<Physics>();
    auto& rb  = m_registry.get<RigidBody>(tower);

    rb.rigid_body->getCollider(0)->setIsSimulationCollider(true);

    m_registry.emplace_or_replace<RigidBody>(tower, rb);
    m_registry.emplace<Health>(tower, 100.0f);
    m_registry.remove<PlacementMode>(tower);
    m_registry.remove<ImguiDrawable>(tower);

    m_ghost_tower = entt::null;
    m_registry.ctx().get<entt::dispatcher&>().trigger(TowerCreated{});
}

void ActionSystem::tower_placement_mode(TowerPlacementMode m)
{
    auto intersect = get_mouse_ray();

    if (m_registry.valid(m_ghost_tower)) {
        if (!m.enable) {
            auto& phy = m_registry.ctx().get<Physics>();
            auto& rb  = m_registry.get<RigidBody>(m_ghost_tower);

            phy.world->destroyRigidBody(rb.rigid_body);
            m_registry.destroy(m_ghost_tower);
            m_ghost_tower = entt::null;
            return;
        }

        m_registry.patch<Transform3D>(m_ghost_tower, [intersect, this](Transform3D& t) {
            t.position.x = intersect.x;
            t.position.z = intersect.z;
            m_registry.patch<RigidBody>(m_ghost_tower,
                [intersect, t](RigidBody& rb) { rb.rigid_body->setTransform(t.to_rp3d()); });
        });
    } else if (m.enable) {
        auto& phy     = m_registry.ctx().get<Physics>();
        m_ghost_tower = m_registry.create();
        m_registry.emplace<SceneObject>(m_ghost_tower, "tower_model"_hs);
        const auto& t = m_registry.emplace<Transform3D>(m_ghost_tower,
            glm::vec3(intersect.x, 0.0f, intersect.z),
            glm::vec3(0.0f),
            glm::vec3(0.1f));
        m_registry.emplace<PlacementMode>(m_ghost_tower);
        m_registry.emplace<ImguiDrawable>(m_ghost_tower, "Ghost Tower");

        auto* rb       = phy.world->createRigidBody(t.to_rp3d());
        auto* box      = phy.common.createBoxShape(rp3d::Vector3(0.35f, 0.65f, 0.35f));
        auto* collider = rb->addCollider(box, rp3d::Transform::identity());

        rb->enableGravity(false);
        rb->setType(rp3d::BodyType::DYNAMIC);
        collider->setIsTrigger(true);
        rb->setUserData(&m_ghost_tower);

#if WATO_DEBUG
        rb->setIsDebugEnabled(true);
#endif

        m_registry.emplace<RigidBody>(m_ghost_tower, rb);
    }
}
