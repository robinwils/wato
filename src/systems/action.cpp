#include "core/action.hpp"

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
#include "config.h"
#include "core/cache.hpp"
#include "core/ray.hpp"
#include "entt/core/hashed_string.hpp"
#include "entt/entity/entity.hpp"
#include "entt/entity/fwd.hpp"
#include "entt/signal/dispatcher.hpp"
#include "entt/signal/fwd.hpp"
#include "glm/trigonometric.hpp"
#include "input/input.hpp"
#include "reactphysics3d/components/RigidBodyComponents.h"
#include "reactphysics3d/engine/PhysicsWorld.h"
#include "reactphysics3d/mathematics/Transform.h"
#include "renderer/plane_primitive.hpp"
#include "systems/systems.hpp"

using namespace entt::literals;

void ActionSystem::InitListeners(Input& aInput)
{
    auto& dispatcher = mRegistry->ctx().get<entt::dispatcher&>();
    dispatcher.sink<CameraMovement>().connect<&ActionSystem::cameraMovement>(this);
    dispatcher.sink<BuildTower>().connect<&ActionSystem::buildTower>(this);
    dispatcher.sink<TowerPlacementMode>().connect<&ActionSystem::towerPlacementMode>(this);
    dispatcher.sink<TowerCreated>().connect<&Input::ExitTowerPlacementMode>(aInput);
}

void ActionSystem::UdpateWinSize(int aW, int aH)
{
    mWinWidth  = aW;
    mWinHeight = aH;
}

void ActionSystem::cameraMovement(CameraMovement aCm)
{
    for (auto&& [entity, cam, t] : mRegistry->view<Camera, Transform3D>().each()) {
        float const speed = cam.Speed * aCm.Delta;
        switch (aCm.Action) {
            case CameraMovement::CameraForward:
                t.Position += speed * cam.Front;
                break;
            case CameraMovement::CameraLeft:
                t.Position += speed * cam.Right();
                break;
            case CameraMovement::CameraBack:
                t.Position -= speed * cam.Front;
                break;
            case CameraMovement::CameraRight:
                t.Position -= speed * cam.Right();
                break;
            case CameraMovement::CameraZoom:
                if ((t.Position.y >= 1.0F && speed > 0.0F)
                    || (t.Position.y <= 10.0F && speed < 0.0F)) {
                    t.Position -= speed * cam.Up;
                }
                break;
            default:
                throw std::runtime_error("wrong action for camera movement");
        }
        break;
    }
}

glm::vec3 ActionSystem::getMouseRay() const
{
    const auto& input = mRegistry->ctx().get<Input&>();

    for (auto&& [_, cam, tcam] : mRegistry->view<Camera, Transform3D>().each()) {
        for (auto&& [_, t, obj] : mRegistry->view<Transform3D, SceneObject, Tile>().each()) {
            auto ray =
                Ray(tcam.Position, input.WorldMousePos(cam, tcam.Position, mWinWidth, mWinHeight));

            const auto& primitives = WATO_MODEL_CACHE[obj.model_hash];
            BX_ASSERT(primitives->size() == 1, "plane should have 1 primitive");
            const auto* plane = dynamic_cast<PlanePrimitive*>(primitives->back());

            float const d = ray.IntersectPlane(plane->Normal(t.Rotation));
            return ray.Orig + d * ray.Dir;
        }
    }
    throw std::runtime_error("should not be here, no terrain or camera was instanced");
}

void ActionSystem::buildTower(BuildTower /*bt*/)
{
    if (!mCanBuild) {
        return;
    }
    auto tower = mGhostTower;

    BX_ASSERT(mRegistry->valid(tower), "ghost tower must be valid");
    auto& rb = mRegistry->get<RigidBody>(tower);

    rb.rigid_body->getCollider(0)->setIsSimulationCollider(true);

    mRegistry->emplace_or_replace<RigidBody>(tower, rb);
    mRegistry->emplace<Health>(tower, 100.0F);
    mRegistry->remove<PlacementMode>(tower);
    mRegistry->remove<ImguiDrawable>(tower);

    mGhostTower = entt::null;
    mRegistry->ctx().get<entt::dispatcher&>().trigger(TowerCreated{});
}

void ActionSystem::towerPlacementMode(TowerPlacementMode aM)
{
    auto intersect = getMouseRay();

    if (mRegistry->valid(mGhostTower)) {
        if (!aM.Enable) {
            auto& phy = mRegistry->ctx().get<Physics>();
            auto& rb  = mRegistry->get<RigidBody>(mGhostTower);

            phy.World->destroyRigidBody(rb.rigid_body);
            mRegistry->destroy(mGhostTower);
            mGhostTower = entt::null;
            return;
        }

        mRegistry->patch<Transform3D>(mGhostTower, [intersect, this](Transform3D& aT) {
            aT.Position.x = intersect.x;
            aT.Position.z = intersect.z;
            mRegistry->patch<RigidBody>(mGhostTower,
                [aT](RigidBody& aRb) { aRb.rigid_body->setTransform(aT.ToRP3D()); });
        });
    } else if (aM.Enable) {
        auto& phy   = mRegistry->ctx().get<Physics>();
        mGhostTower = mRegistry->create();
        mRegistry->emplace<SceneObject>(mGhostTower, "tower_model"_hs);
        const auto& t = mRegistry->emplace<Transform3D>(mGhostTower,
            glm::vec3(intersect.x, 0.0F, intersect.z),
            glm::vec3(0.0F),
            glm::vec3(0.1F));
        mRegistry->emplace<PlacementMode>(mGhostTower);
        mRegistry->emplace<ImguiDrawable>(mGhostTower, "Ghost Tower");

        auto* rb       = phy.World->createRigidBody(t.ToRP3D());
        auto* box      = phy.Common.createBoxShape(rp3d::Vector3(0.35F, 0.65F, 0.35F));
        auto* collider = rb->addCollider(box, rp3d::Transform::identity());

        rb->enableGravity(false);
        rb->setType(rp3d::BodyType::DYNAMIC);
        collider->setIsTrigger(true);
        rb->setUserData(&mGhostTower);

#if WATO_DEBUG
        rb->setIsDebugEnabled(true);
#endif

        mRegistry->emplace<RigidBody>(mGhostTower, rb);
    }
}
