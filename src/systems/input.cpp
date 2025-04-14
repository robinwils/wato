#include "input/input.hpp"

#include <fmt/base.h>

#include "components/health.hpp"
#include "components/imgui.hpp"
#include "components/placement_mode.hpp"
#include "components/rigid_body.hpp"
#include "components/scene_object.hpp"
#include "components/tile.hpp"
#include "components/transform3d.hpp"
#include "core/cache.hpp"
#include "core/event/creep_spawn.hpp"
#include "core/event_handler.hpp"
#include "core/net/enet_client.hpp"
#include "core/net/net.hpp"
#include "core/physics.hpp"
#include "core/ray.hpp"
#include "core/window.hpp"
#include "registry/registry.hpp"
#include "renderer/plane_primitive.hpp"
#include "systems/input.hpp"

void PlayerInputSystem::operator()(Registry& aRegistry, const float aDeltaTime)
{
    cameraInput(aRegistry, aDeltaTime);

    RingBuffer<Input, 128>& rbuf      = aRegistry.ctx().get<WatoWindow&>().GetInput();
    std::optional<Input>&   prevInput = rbuf.Previous();
    Input&                  input     = rbuf.Latest();

    if (!input.IsPlacementMode()) {
        if (input.KeyboardState.IsKeyPressed(Keyboard::B)) {
            input.EnterTowerPlacementMode();
            towerPlacementMode(aRegistry, true);
        }

        // Creeps
        if (input.KeyboardState.IsKeyPressed(Keyboard::C) && prevInput
            && prevInput->KeyboardState.IsKeyPressed(Keyboard::C)) {
            creepSpawn(aRegistry);
        }
    } else {
        towerPlacementMode(aRegistry, true);
        if (input.KeyboardState.IsKeyPressed(Keyboard::Escape)) {
            input.ExitTowerPlacementMode();
            towerPlacementMode(aRegistry, false);
        }

        if (input.IsMouseButtonPressed(Mouse::Left)
            && !prevInput->IsMouseButtonPressed(Mouse::Left)) {
            buildTower(aRegistry);
        }
    }
}

void PlayerInputSystem::cameraInput(Registry& aRegistry, const float aDeltaTime)
{
    RingBuffer<Input, 128>& rbuf  = aRegistry.ctx().get<WatoWindow&>().GetInput();
    Input&                  input = rbuf.Latest();

    for (auto&& [entity, cam, t] : aRegistry.view<Camera, Transform3D>().each()) {
        float const speed = cam.Speed * aDeltaTime;

        if (input.KeyboardState.IsKeyPressed(Keyboard::W)
            || input.KeyboardState.IsKeyRepeat(Keyboard::W)) {
            t.Position += speed * cam.Front;
        }
        if (input.KeyboardState.IsKeyPressed(Keyboard::A)
            || input.KeyboardState.IsKeyRepeat(Keyboard::A)) {
            t.Position += speed * cam.Right();
        }
        if (input.KeyboardState.IsKeyPressed(Keyboard::S)
            || input.KeyboardState.IsKeyRepeat(Keyboard::S)) {
            t.Position -= speed * cam.Front;
        }
        if (input.KeyboardState.IsKeyPressed(Keyboard::D)
            || input.KeyboardState.IsKeyRepeat(Keyboard::D)) {
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

glm::vec3 PlayerInputSystem::getMouseRay(Registry& aRegistry) const
{
    auto&                   window = aRegistry.ctx().get<WatoWindow&>();
    RingBuffer<Input, 128>& rbuf   = window.GetInput();
    Input&                  input  = rbuf.Latest();

    for (auto&& [_, cam, tcam] : aRegistry.view<Camera, Transform3D>().each()) {
        for (auto&& [_, t, obj] : aRegistry.view<Transform3D, SceneObject, Tile>().each()) {
            auto ray = Ray(tcam.Position,
                input.WorldMousePos(cam,
                    tcam.Position,
                    window.Width<float>(),
                    window.Height<float>()));

            const auto& primitives = WATO_MODEL_CACHE[obj.model_hash];
            BX_ASSERT(primitives->size() == 1, "plane should have 1 primitive");
            const auto* plane = dynamic_cast<PlanePrimitive*>(primitives->back());

            float const d = ray.IntersectPlane(plane->Normal(t.Orientation));
            return ray.Orig + d * ray.Dir;
        }
    }
    throw std::runtime_error("should not be here, no terrain or camera was instanced");
}

void PlayerInputSystem::towerPlacementMode(Registry& aRegistry, bool aEnable)
{
    auto intersect = getMouseRay(aRegistry);

    auto& phy               = aRegistry.ctx().get<Physics&>();
    auto  placementModeView = aRegistry.view<PlacementMode>();

    if (!placementModeView->empty()) {
        for (auto ghostTower : placementModeView) {
            if (!aEnable) {
                aRegistry.destroy(ghostTower);
                return;
            }

            aRegistry.patch<Transform3D>(ghostTower,
                [intersect, &aRegistry, ghostTower](Transform3D& aT) {
                    aT.Position.x = intersect.x;
                    aT.Position.z = intersect.z;
                    aRegistry.patch<RigidBody>(ghostTower,
                        [aT](RigidBody& aRb) { aRb.RigidBody->setTransform(aT.ToRP3D()); });
                });
        }
    } else if (aEnable) {
        auto ghostTower = aRegistry.create();
        aRegistry.emplace<SceneObject>(ghostTower, "tower_model"_hs);
        const auto& t = aRegistry.emplace<Transform3D>(ghostTower,
            glm::vec3(intersect.x, 0.0F, intersect.z),
            glm::identity<glm::quat>(),
            glm::vec3(0.1F));
        aRegistry.emplace<PlacementMode>(ghostTower);
        aRegistry.emplace<ImguiDrawable>(ghostTower, "Ghost Tower");

        rp3d::RigidBody* body = phy.CreateRigidBody(ghostTower,
            aRegistry,
            RigidBodyParams{.Type = rp3d::BodyType::DYNAMIC,
                .Transform        = t.ToRP3D(),
                .GravityEnabled   = false});
        phy.AddBoxCollider(body, rp3d::Vector3(0.35F, 0.65F, 0.35F), true);
    }
}

void PlayerInputSystem::buildTower(Registry& aRegistry)
{
    RingBuffer<Input, 128>& rbuf  = aRegistry.ctx().get<WatoWindow&>().GetInput();
    Input&                  input = rbuf.Latest();

    if (!input.IsAbleToBuild()) {
        return;
    }

    for (auto tower : aRegistry.view<PlacementMode>()) {
        auto& rb = aRegistry.get<RigidBody>(tower);

        rb.RigidBody->getCollider(0)->setIsSimulationCollider(true);
        rb.RigidBody->setType(rp3d::BodyType::STATIC);

        aRegistry.emplace<Health>(tower, 100.0F);
        aRegistry.remove<PlacementMode>(tower);
        aRegistry.remove<ImguiDrawable>(tower);
    }
    input.ExitTowerPlacementMode();
}

void PlayerInputSystem::creepSpawn(Registry& aRegistry)
{
    auto& netClient = aRegistry.ctx().get<ENetClient&>();
    netClient.EnqueueSend(new NetEvent(CreepSpawnEvent()));
}
