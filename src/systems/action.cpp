#include "systems/action.hpp"

#include <variant>

#include "components/animator.hpp"
#include "components/camera.hpp"
#include "components/creep.hpp"
#include "components/health.hpp"
#include "components/imgui.hpp"
#include "components/path.hpp"
#include "components/placement_mode.hpp"
#include "components/rigid_body.hpp"
#include "components/scene_object.hpp"
#include "components/spawner.hpp"
#include "components/transform3d.hpp"
#include "components/velocity.hpp"
#include "core/graph.hpp"
#include "core/physics/physics.hpp"
#include "core/state.hpp"
#include "core/sys/log.hpp"
#include "core/tower_building_handler.hpp"
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
    auto& graph = aRegistry.ctx().get<Graph&>();

    for (auto&& [entity, spawnTransform] : aRegistry.view<Spawner, Transform3D>().each()) {
        auto creep = aRegistry.create();
        aRegistry.emplace<Transform3D>(
            creep,
            spawnTransform.Position,
            glm::identity<glm::quat>(),
            glm::vec3(0.5f));

        aRegistry.emplace<Health>(creep, 100.0f);
        aRegistry.emplace<Creep>(creep, aPayload.Type);
        aRegistry.emplace<SceneObject>(creep, "phoenix"_hs);
        aRegistry.emplace<ImguiDrawable>(creep, "phoenix", true);
        aRegistry.emplace<Animator>(creep, 0.0f, "Take 001");
        aRegistry.emplace<Path>(
            creep,
            GraphCell::FromWorldPoint(spawnTransform.Position),
            graph.GetNextCell(GraphCell::FromWorldPoint(spawnTransform.Position)));

        aRegistry.emplace<RigidBody>(
            creep,
            RigidBody{
                .Params =
                    RigidBodyParams{
                        .Type           = rp3d::BodyType::KINEMATIC,
                        .Velocity       = 0.4f,
                        .Direction      = glm::vec3(0.0f),
                        .GravityEnabled = false,
                    },
            });
        aRegistry.emplace<Collider>(
            creep,
            Collider{
                .Params =
                    ColliderParams{
                        .CollisionCategoryBits = Category::Entities,
                        .CollideWithMaskBits   = 0,
                        .IsTrigger             = false,
                        .Offset                = Transform3D{},
                        .ShapeParams =
                            CapsuleShapeParams{
                                .Radius = 0.1f,
                                .Height = 0.05f,
                            },
                    },
            });
    }
}

void DefaultContextHandler::operator()(Registry& aRegistry, const PlacementModePayload& aPayload)
{
    WATO_TRACE(aRegistry, "entering placement mode");
    auto& contextStack = aRegistry.ctx().get<ActionContextStack&>();

    ActionContext placementCtx{
        .State    = ActionContext::State::Placement,
        .Bindings = ActionBindings::PlacementDefaults(),
        .Payload  = PlacementModePayload{.CanBuild = true, .Tower = aPayload.Tower}};

    contextStack.push_front(std::move(placementCtx));

    auto ghostTower = aRegistry.create();
    WATO_DBG(aRegistry, "created ghost tower {}", ghostTower);
    aRegistry.emplace<SceneObject>(ghostTower, "tower_model"_hs);
    aRegistry.emplace<Transform3D>(
        ghostTower,
        glm::vec3(0.0f),
        glm::identity<glm::quat>(),
        glm::vec3(0.1f));
    aRegistry.emplace<PlacementMode>(ghostTower);
}

void DefaultContextHandler::ExitPlacement(Registry& aRegistry)
{
    auto& contextStack = aRegistry.ctx().get<ActionContextStack&>();
    if (contextStack.size() > 1) {
        if (contextStack.front().State == ActionContext::State::Placement) {
            auto view = aRegistry.view<PlacementMode>();
            for (auto entity : view) {
                aRegistry.destroy(entity);
                WATO_DBG(aRegistry, "destroyed ghost tower {}", entity);
            }
        }
        contextStack.pop_front();
    }
}

void PlacementModeContextHandler::operator()(Registry& aRegistry, const BuildTowerPayload& aPayload)
{
    auto& phy = aRegistry.ctx().get<Physics>();

    for (const auto&& [tower, pm, t] : aRegistry.view<PlacementMode, Transform3D>().each()) {
        TowerBuildingHandler handler(WATO_REG_LOGGER(aRegistry));

        RigidBody body = RigidBody{
            .Params =
                RigidBodyParams{
                    .Type           = rp3d::BodyType::STATIC,
                    .Velocity       = 0.0f,
                    .Direction      = glm::vec3(0.0f),
                    .GravityEnabled = false,
                },
        };
        Collider collider = Collider{
            .Params =
                ColliderParams{
                    .CollisionCategoryBits = Category::Entities,
                    .CollideWithMaskBits   = Category::Terrain | Category::Entities,
                    .IsTrigger             = false,
                    .Offset                = Transform3D{},
                    .ShapeParams =
                        BoxShapeParams{
                            .HalfExtents = glm::vec3(0.35f, 0.65f, 0.35f),
                        },
                },
        };
        body.Body       = phy.CreateRigidBody(body.Params, t);
        collider.Handle = phy.AddCollider(body.Body, collider.Params);

        phy.World()->testOverlap(body.Body, handler);
        if (!handler.CanBuildTower) {
            phy.World()->destroyRigidBody(body.Body);
            return;
        }

        aRegistry.emplace<Tower>(tower, aPayload.Tower);
        aRegistry.emplace<RigidBody>(tower, body);
        aRegistry.emplace<Collider>(tower, collider);
        aRegistry.emplace<Health>(tower, 100.0f);
        aRegistry.remove<ImguiDrawable>(tower);
        // remove component so that ExitPlacement does not destroy the entity
        aRegistry.remove<PlacementMode>(tower);
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

    auto& t = aRegistry.emplace<Transform3D>(tower, aPayload.Position);

    RigidBody body = RigidBody{
        .Params =
            RigidBodyParams{
                .Type           = rp3d::BodyType::STATIC,
                .Velocity       = 0.0f,
                .Direction      = glm::vec3(0.0f),
                .GravityEnabled = false,
            },
    };
    Collider collider = Collider{
        .Params =
            ColliderParams{
                .CollisionCategoryBits = Category::Entities,
                .CollideWithMaskBits   = Category::Terrain | Category::Entities,
                .IsTrigger             = false,
                .Offset                = Transform3D{},
                .ShapeParams =
                    BoxShapeParams{
                        .HalfExtents = glm::vec3(0.35f, 0.65f, 0.35f),
                    },
            },
    };

    body.Body       = phy.CreateRigidBody(body.Params, t);
    collider.Handle = phy.AddCollider(body.Body, collider.Params);

    TowerBuildingHandler handler(WATO_REG_LOGGER(aRegistry));
    phy.World()->testOverlap(body.Body, handler);

    if (!handler.CanBuildTower) {
        phy.World()->destroyRigidBody(body.Body);
        aRegistry.destroy(tower);
        spdlog::error("tower at {} invalidated", t.Position);
        return;
    }
    WATO_INFO(aRegistry, "tower at {} validated", t.Position);
    aRegistry.emplace<Tower>(tower, aPayload.Tower);
    aRegistry.emplace<RigidBody>(tower, body);
    aRegistry.emplace<Collider>(tower, collider);
}

template <typename Derived>
void ActionSystem<Derived>::handleAction(
    Registry&     aRegistry,
    const Action& aAction,
    const float   aDeltaTime)
{
    WATO_TRACE(aRegistry, "handling {}", aAction);
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
    auto&       buf           = aRegistry.ctx().get<GameStateBuffer&>();
    ActionsType latestActions = buf.Latest().Actions;

    if (!latestActions.empty()) {
        WATO_TRACE(aRegistry, "processing {} actions", latestActions.size());
    }

    for (const Action& action : latestActions) {
        if (action.Tag != aFilterTag || action.IsProcessed) {
            continue;
        }
        handleAction(aRegistry, action, aDeltaTime);
    }

    for (Action& action : latestActions) {
        action.IsProcessed = true;
    }
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
