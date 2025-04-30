#include "systems/action.hpp"

#include <fmt/base.h>

#include "components/creep.hpp"
#include "components/health.hpp"
#include "components/imgui.hpp"
#include "components/placement_mode.hpp"
#include "components/rigid_body.hpp"
#include "components/scene_object.hpp"
#include "components/transform3d.hpp"
#include "core/physics.hpp"
#include "input/action.hpp"
#include "registry/registry.hpp"

template <typename Derived>
void ActionSystem<Derived>::handleAction(
    Registry&     aRegistry,
    const Action& aAction,
    const float   aDeltaTime)
{
    auto&       contextStack = aRegistry.ctx().get<ActionContextStack&>();
    const auto& currentCtx   = contextStack.front();

    switch (currentCtx.State) {
        case ActionContext::State::Default:
            handleDefaultContext(aRegistry, aAction, aDeltaTime);
            break;
        case ActionContext::State::Placement:
            handlePlacementContext(aRegistry, aAction, aDeltaTime);
            break;
    }
}

template <typename Derived>
void ActionSystem<Derived>::processActions(
    Registry&   aRegistry,
    ActionTag   aFilterTag,
    const float aDeltaTime)
{
    auto&         abuf          = aRegistry.ctx().get<ActionBuffer&>();
    PlayerActions latestActions = abuf.Latest();

    for (const Action& action : latestActions.Actions) {
        // fmt::println("{}", action.String());
        if (action.Tag != aFilterTag) {
            continue;
        }
        handleAction(aRegistry, action, aDeltaTime);
    }
}

template <typename Derived>
void ActionSystem<Derived>::handleDefaultContext(
    Registry&     aRegistry,
    const Action& aAction,
    const float   aDeltaTime)
{
    switch (aAction.Type) {
        case ActionType::EnterPlacementMode:
            if (const auto* payload = std::get_if<PlacementModePayload>(&aAction.Payload)) {
                transitionToPlacement(aRegistry, *payload);
            }
            break;
        case ActionType::Move:
            if (const auto* payload = std::get_if<MovePayload>(&aAction.Payload)) {
                handleMovement(aRegistry, *payload, aDeltaTime);
            }
            break;
        case ActionType::SendCreep:
            if (const auto* payload = std::get_if<SendCreepPayload>(&aAction.Payload)) {
                auto& phy   = aRegistry.ctx().get<Physics&>();
                auto  creep = aRegistry.create();
                auto& t     = aRegistry.emplace<Transform3D>(
                    creep,
                    glm::vec3(2.0f, 0.0f, 2.0f),
                    glm::identity<glm::quat>(),
                    glm::vec3(0.1f));

                aRegistry.emplace<Health>(creep, 100.0f);
                aRegistry.emplace<Creep>(creep, payload->Type);

                auto* body = phy.CreateRigidBody(
                    creep,
                    aRegistry,
                    RigidBodyParams{
                        .Type           = rp3d::BodyType::DYNAMIC,
                        .Transform      = t.ToRP3D(),
                        .GravityEnabled = false});
                rp3d::Collider* collider = phy.AddCapsuleCollider(body, 0.1f, 0.05f);
                collider->setCollisionCategoryBits(Category::Entities);

                fmt::println("got a creep !!");
            }
            break;
        default:
            break;
    }
}

template <typename Derived>
void ActionSystem<Derived>::handleMovement(
    Registry&          aRegistry,
    const MovePayload& aPayload,
    const float        aDeltaTime)
{
    for (auto&& [entity, camera, transform] : aRegistry.view<Camera, Transform3D>().each()) {
        float const speed = camera.Speed * aDeltaTime;

        switch (aPayload.Direction) {
            case MoveDirection::Left:
                transform.Position += speed * camera.Right();
                break;
            case MoveDirection::Right:
                transform.Position -= speed * camera.Right();
                break;
            case MoveDirection::Front:
                transform.Position += speed * camera.Front;
                break;
            case MoveDirection::Back:
                transform.Position -= speed * camera.Front;
                break;
            case MoveDirection::Up:
                if (transform.Position.y <= 10.0F) {
                    transform.Position += speed * camera.Up;
                }
                break;
            case MoveDirection::Down:
                if (transform.Position.y >= 1.0F) {
                    transform.Position -= speed * camera.Up;
                }
                break;
        }
    }
}

template <typename Derived>
void ActionSystem<Derived>::handlePlacementContext(
    Registry&     aRegistry,
    const Action& aAction,
    const float   aDeltaTime)
{
    switch (aAction.Type) {
        case ActionType::ExitPlacementMode:
            exitPlacement(aRegistry);
            break;
        case ActionType::Move: {
            if (const auto* payload = std::get_if<MovePayload>(&aAction.Payload)) {
                handleMovement(aRegistry, *payload, aDeltaTime);
            }
            break;
        }
        case ActionType::BuildTower: {
            auto& contextStack = aRegistry.ctx().get<ActionContextStack&>();
            if (auto* payload = std::get_if<PlacementModePayload>(&contextStack.front().Payload);
                !payload->CanBuild) {
                break;
            }
            for (auto tower : aRegistry.view<PlacementMode>()) {
                auto& rb = aRegistry.get<RigidBody>(tower);

                rb.RigidBody->getCollider(0)->setIsSimulationCollider(true);
                rb.RigidBody->getCollider(0)->setCollisionCategoryBits(Category::Entities);
                rb.RigidBody->getCollider(0)->setCollideWithMaskBits(
                    Category::Terrain | Category::PlacementGhostTower);
                rb.RigidBody->setType(rp3d::BodyType::STATIC);

                aRegistry.emplace<Health>(tower, 100.0F);
                aRegistry.remove<PlacementMode>(tower);
                aRegistry.remove<ImguiDrawable>(tower);
            }
            exitPlacement(aRegistry);
            break;
        }
        default:
            break;
    }
}

template <typename Derived>
void ActionSystem<Derived>::transitionToPlacement(
    Registry&                   aRegistry,
    const PlacementModePayload& aPayload)
{
    auto& contextStack = aRegistry.ctx().get<ActionContextStack&>();
    auto& phy          = aRegistry.ctx().get<Physics&>();

    ActionContext placementCtx{
        .State    = ActionContext::State::Placement,
        .Bindings = ActionBindings::PlacementDefaults(),
        .Payload  = PlacementModePayload{.CanBuild = true, .Tower = aPayload.Tower}
    };

    contextStack.push_front(std::move(placementCtx));

    auto ghostTower = aRegistry.create();
    aRegistry.emplace<SceneObject>(ghostTower, "tower_model"_hs);
    aRegistry.emplace<Tower>(ghostTower, aPayload.Tower);
    auto& t = aRegistry.emplace<Transform3D>(
        ghostTower,
        glm::vec3(0.0f),
        glm::identity<glm::quat>(),
        glm::vec3(0.1f));
    aRegistry.emplace<PlacementMode>(ghostTower);
    aRegistry.emplace<ImguiDrawable>(ghostTower, "Ghost Tower");
    rp3d::RigidBody* body = phy.CreateRigidBody(
        ghostTower,
        aRegistry,
        RigidBodyParams{
            .Type           = rp3d::BodyType::DYNAMIC,
            .Transform      = t.ToRP3D(),
            .GravityEnabled = false});
    rp3d::Collider* collider = phy.AddBoxCollider(body, rp3d::Vector3(0.35F, 0.65F, 0.35F), true);
    collider->setCollisionCategoryBits(Category::PlacementGhostTower);
    collider->setCollideWithMaskBits(Category::Terrain | Category::Entities);
}

template <typename Derived>
void ActionSystem<Derived>::exitPlacement(Registry& aRegistry)
{
    auto& contextStack = aRegistry.ctx().get<ActionContextStack&>();
    if (contextStack.size() > 1) {
        if (contextStack.front().State == ActionContext::State::Placement) {
            auto view = aRegistry.view<PlacementMode>();
            for (auto entity : view) {
                aRegistry.destroy(entity);
            }
        }
        contextStack.pop_front();
    }
}

// Explicit template instantiations
template class ActionSystem<RealTimeActionSystem>;
template class ActionSystem<DeterministicActionSystem>;
