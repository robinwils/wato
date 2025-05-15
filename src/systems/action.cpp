#include "systems/action.hpp"

#include <spdlog/spdlog.h>

#include <variant>

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

void DefaultContextHandler::operator()(
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

void DefaultContextHandler::operator()(Registry& aRegistry, const SendCreepPayload& aPayload)
{
    auto& phy   = aRegistry.ctx().get<Physics&>();
    auto  creep = aRegistry.create();
    auto& t     = aRegistry.emplace<Transform3D>(
        creep,
        glm::vec3(2.0f, 0.0f, 2.0f),
        glm::identity<glm::quat>(),
        glm::vec3(0.1f));

    aRegistry.emplace<Health>(creep, 100.0f);
    aRegistry.emplace<Creep>(creep, aPayload.Type);

    auto* body = phy.CreateRigidBody(
        creep,
        aRegistry,
        RigidBodyParams{
            .Type           = rp3d::BodyType::DYNAMIC,
            .Transform      = t.ToRP3D(),
            .GravityEnabled = false});
    rp3d::Collider* collider = phy.AddCapsuleCollider(body, 0.1f, 0.05f);
    collider->setCollisionCategoryBits(Category::Entities);
}

void DefaultContextHandler::operator()(Registry& aRegistry, const PlacementModePayload& aPayload)
{
    spdlog::info("entering placement mode");
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

void DefaultContextHandler::ExitPlacement(Registry& aRegistry)
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

void PlacementModeContextHandler::operator()(Registry& aRegistry, const BuildTowerPayload& aPayload)
{
    auto& contextStack = aRegistry.ctx().get<ActionContextStack&>();
    if (auto* payload = std::get_if<PlacementModePayload>(&contextStack.front().Payload);
        !payload->CanBuild) {
        return;
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
    SPDLOG_DEBUG("exiting placement mode");
    ExitPlacement(aRegistry);
}

void PlacementModeContextHandler::operator()(Registry& aRegistry, const PlacementModePayload&)
{
    SPDLOG_DEBUG("exiting placement mode");
    ExitPlacement(aRegistry);
}

void ServerContextHandler::operator()(Registry& aRegistry, const BuildTowerPayload& aPayload)
{
    auto  tower = aRegistry.create();
    auto& phy   = aRegistry.ctx().get<Physics>();

    aRegistry.emplace<SceneObject>(tower, "tower_model"_hs);
    aRegistry.emplace<Tower>(tower, aPayload.Tower);

    auto& t = aRegistry.emplace<Transform3D>(
        tower,
        aPayload.Position,
        glm::identity<glm::quat>(),
        glm::vec3(0.1f));
    aRegistry.emplace<PlacementMode>(tower);
    aRegistry.emplace<ImguiDrawable>(tower, "Ghost Tower");

    rp3d::RigidBody* body = phy.CreateRigidBody(
        tower,
        aRegistry,
        RigidBodyParams{
            .Type           = rp3d::BodyType::DYNAMIC,
            .Transform      = t.ToRP3D(),
            .GravityEnabled = false});
    rp3d::Collider* collider = phy.AddBoxCollider(body, rp3d::Vector3(0.35F, 0.65F, 0.35F), true);

    collider->setIsSimulationCollider(true);
    collider->setCollisionCategoryBits(Category::Entities);
    collider->setCollideWithMaskBits(Category::Terrain | Category::PlacementGhostTower);
    body->setType(rp3d::BodyType::STATIC);

    TowerBuildingHandler handler;
    phy.World()->testCollision(body, handler);

    if (!handler.CanBuildTower) {
        phy.DeleteRigidBody(aRegistry, tower);
    }
}

template <typename Derived>
void ActionSystem<Derived>::handleAction(
    Registry&     aRegistry,
    const Action& aAction,
    const float   aDeltaTime)
{
    spdlog::info("handlig {}", aAction);
    auto&       contextStack = aRegistry.ctx().get<ActionContextStack&>();
    const auto& currentCtx   = contextStack.front();

    switch (currentCtx.State) {
        case ActionContext::State::Default: {
            DefaultContextHandler handler;
            handleContext(aRegistry, aAction, handler, aDeltaTime);
            break;
        }
        case ActionContext::State::Placement: {
            PlacementModeContextHandler handler;
            handleContext(aRegistry, aAction, handler, aDeltaTime);
            break;
        }
        case ActionContext::State::Server: {
            ServerContextHandler handler;
            handleContext(aRegistry, aAction, handler, aDeltaTime);
            break;
        }
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

    if (!latestActions.Actions.empty()) {
        spdlog::info("processing {} actions", latestActions.Actions.size());
    }
    for (const Action& action : latestActions.Actions) {
        if (action.Tag != aFilterTag) {
            continue;
        }
        handleAction(aRegistry, action, aDeltaTime);
    }

    // TODO: mark action as processed, otherwise the real time loop will re execute it
}

template <typename Derived>
void ActionSystem<Derived>::handleContext(
    Registry&             aRegistry,
    const Action&         aAction,
    ActionContextHandler& aCtxHandler,
    const float           aDeltaTime)
{
    std::visit(
        VariantVisitor{
            [&](const PlacementModePayload& aPayload) { aCtxHandler(aRegistry, aPayload); },
            [&](const MovePayload& aPayload) { aCtxHandler(aRegistry, aPayload, aDeltaTime); },
            [&](const BuildTowerPayload& aPayload) { aCtxHandler(aRegistry, aPayload); },
            [&](const SendCreepPayload& aPayload) { aCtxHandler(aRegistry, aPayload); },
        },
        aAction.Payload);
}

// Explicit template instantiations
template class ActionSystem<RealTimeActionSystem>;
template class ActionSystem<DeterministicActionSystem>;
template class ActionSystem<ServerActionSystem>;
