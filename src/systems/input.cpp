#include "input/input.hpp"

#include "components/health.hpp"
#include "components/imgui.hpp"
#include "components/physics.hpp"
#include "components/placement_mode.hpp"
#include "components/rigid_body.hpp"
#include "components/scene_object.hpp"
#include "components/tile.hpp"
#include "components/transform3d.hpp"
#include "core/action.hpp"
#include "core/cache.hpp"
#include "core/ray.hpp"
#include "core/registry.hpp"
#include "core/window.hpp"
#include "entt/signal/dispatcher.hpp"
#include "renderer/plane_primitive.hpp"
#include "systems/input.hpp"

void PlayerInputSystem::operator()(Registry& aRegistry, const float aDeltaTime, WatoWindow& aWin)
{
    cameraInput(aRegistry, aDeltaTime);

    auto& input = aRegistry.ctx().get<Input&>();
    if (input.IsKeyPressed(Keyboard::B) && !input.IsPrevKeyPressed(Keyboard::B)
        && !input.IsMouseButtonPressed(Mouse::Left)) {
        if (!input.IsPlacementMode()) {
            input.EnterTowerPlacementMode();
        }
        towerPlacementMode(aRegistry, aWin, true);
    }

    if (input.IsPlacementMode()) {
        towerPlacementMode(aRegistry, aWin, true);
    }

    if (input.IsPlacementMode()) {
        if (input.IsKeyPressed(Keyboard::Escape)) {
            input.ExitTowerPlacementMode();
            towerPlacementMode(aRegistry, aWin, false);
        }

        if (input.IsMouseButtonPressed(Mouse::Left)) {
            buildTower(aRegistry);
            input.ExitTowerPlacementMode();
        }
    }
}

void PlayerInputSystem::cameraInput(Registry& aRegistry, const float aDeltaTime)
{
    auto& input = aRegistry.ctx().get<Input&>();
    for (auto&& [entity, cam, t] : aRegistry.view<Camera, Transform3D>().each()) {
        float const speed = cam.Speed * aDeltaTime;

        if (input.IsKeyPressed(Keyboard::W) || input.IsKeyRepeat(Keyboard::W)) {
            t.Position += speed * cam.Front;
        }
        if (input.IsKeyPressed(Keyboard::A) || input.IsKeyRepeat(Keyboard::A)) {
            t.Position += speed * cam.Right();
        }
        if (input.IsKeyPressed(Keyboard::S) || input.IsKeyRepeat(Keyboard::S)) {
            t.Position -= speed * cam.Front;
        }
        if (input.IsKeyPressed(Keyboard::D) || input.IsKeyRepeat(Keyboard::D)) {
            t.Position -= speed * cam.Right();
        }
        if (input.MouseState.Scroll.y != 0) {
            if ((t.Position.y >= 1.0F && speed > 0.0F) || (t.Position.y <= 10.0F && speed < 0.0F)) {
                t.Position -= (input.MouseState.Scroll.y * speed) * cam.Up;
            }
            input.MouseState.Scroll.y = 0;
        }
    }
}

glm::vec3 PlayerInputSystem::getMouseRay(Registry& aRegistry, WatoWindow& aWin) const
{
    const auto& input = aRegistry.ctx().get<Input&>();

    for (auto&& [_, cam, tcam] : aRegistry.view<Camera, Transform3D>().each()) {
        for (auto&& [_, t, obj] : aRegistry.view<Transform3D, SceneObject, Tile>().each()) {
            auto ray = Ray(tcam.Position,
                input.WorldMousePos(cam, tcam.Position, aWin.Width<float>(), aWin.Height<float>()));

            const auto& primitives = WATO_MODEL_CACHE[obj.model_hash];
            BX_ASSERT(primitives->size() == 1, "plane should have 1 primitive");
            const auto* plane = dynamic_cast<PlanePrimitive*>(primitives->back());

            float const d = ray.IntersectPlane(plane->Normal(t.Orientation));
            return ray.Orig + d * ray.Dir;
        }
    }
    throw std::runtime_error("should not be here, no terrain or camera was instanced");
}

void PlayerInputSystem::towerPlacementMode(Registry& aRegistry, WatoWindow& aWin, bool aEnable)
{
    auto intersect = getMouseRay(aRegistry, aWin);

    auto placementModeView = aRegistry.view<PlacementMode>();

    if (!placementModeView->empty()) {
        for (auto ghostTower : placementModeView) {
            if (!aEnable) {
                auto& phy = aRegistry.ctx().get<Physics>();
                auto& rb  = aRegistry.get<RigidBody>(ghostTower);

                phy.World->destroyRigidBody(rb.rigid_body);
                aRegistry.destroy(ghostTower);
                ghostTower = entt::null;
                return;
            }

            aRegistry.patch<Transform3D>(ghostTower,
                [intersect, &aRegistry, ghostTower](Transform3D& aT) {
                    aT.Position.x = intersect.x;
                    aT.Position.z = intersect.z;
                    aRegistry.patch<RigidBody>(ghostTower,
                        [aT](RigidBody& aRb) { aRb.rigid_body->setTransform(aT.ToRP3D()); });
                });
        }
    } else if (aEnable) {
        auto& phy        = aRegistry.ctx().get<Physics>();
        auto  ghostTower = aRegistry.create();
        aRegistry.emplace<SceneObject>(ghostTower, "tower_model"_hs);
        const auto& t = aRegistry.emplace<Transform3D>(ghostTower,
            glm::vec3(intersect.x, 0.0F, intersect.z),
            glm::identity<glm::quat>(),
            glm::vec3(0.1F));
        aRegistry.emplace<PlacementMode>(ghostTower);
        aRegistry.emplace<ImguiDrawable>(ghostTower, "Ghost Tower");

        auto* rb       = phy.World->createRigidBody(t.ToRP3D());
        auto* box      = phy.Common.createBoxShape(rp3d::Vector3(0.35F, 0.65F, 0.35F));
        auto* collider = rb->addCollider(box, rp3d::Transform::identity());

        rb->enableGravity(false);
        rb->setType(rp3d::BodyType::DYNAMIC);
        collider->setIsTrigger(true);
        rb->setUserData(&ghostTower);

#if WATO_DEBUG
        rb->setIsDebugEnabled(true);
#endif

        aRegistry.emplace<RigidBody>(ghostTower, rb);
    }
}

void PlayerInputSystem::buildTower(Registry& aRegistry)
{
    if (!aRegistry.ctx().get<Input&>().IsAbleToBuild()) {
        return;
    }

    for (auto tower : aRegistry.view<PlacementMode>()) {
        auto& rb = aRegistry.get<RigidBody>(tower);

        rb.rigid_body->getCollider(0)->setIsSimulationCollider(true);

        aRegistry.emplace_or_replace<RigidBody>(tower, rb);
        aRegistry.emplace<Health>(tower, 100.0F);
        aRegistry.remove<PlacementMode>(tower);
        aRegistry.remove<ImguiDrawable>(tower);
    }
}
