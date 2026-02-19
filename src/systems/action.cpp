#include "systems/action.hpp"

#include <variant>

#include "components/animator.hpp"
#include "components/camera.hpp"
#include "components/creep.hpp"
#include "components/game.hpp"
#include "components/health.hpp"
#include "components/imgui.hpp"
#include "components/model_rotation_offset.hpp"
#include "components/net.hpp"
#include "components/path.hpp"
#include "components/placement_mode.hpp"
#include "components/rigid_body.hpp"
#include "components/scene_object.hpp"
#include "components/spawner.hpp"
#include "components/tower_attack.hpp"
#include "components/transform3d.hpp"
#include "components/velocity.hpp"
#include "core/graph.hpp"
#include "core/net/enet_server.hpp"
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
            case MoveDirection::Count:
                break;
        }
    }
}

void DefaultContextHandler::operator()(Registry& aRegistry, SendCreepPayload& aPayload)
{
    // No client-side prediction - server will create and broadcast creep to all clients
    // Client just sends the request
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
    glm::vec3 startPos{0.0f};
    if (auto** ip = aRegistry.ctx().find<const Input*>()) {
        if (const auto& hit = (*ip)->MouseWorldIntersect()) {
            startPos = *hit;
        }
    }
    aRegistry
        .emplace<Transform3D>(ghostTower, startPos, glm::identity<glm::quat>(), glm::vec3(0.1f));
    aRegistry.emplace<PlacementMode>(ghostTower);
    aRegistry.emplace<ImguiDrawable>(ghostTower, "Placement ghost tower", true);
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

void PlacementModeContextHandler::operator()(Registry& aRegistry, BuildTowerPayload& aPayload)
{
    auto& phy    = aRegistry.ctx().get<Physics>();
    auto& player = aRegistry.ctx().get<Player>("player"_hs);
    auto& sender = aRegistry.get<Player>(GetSenderFor(aRegistry, player.ID));

    for (const auto&& [tower, pm, t] : aRegistry.view<PlacementMode, Transform3D>().each()) {
        // Client-side validation - create temporary body for collision test
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
                    .CollisionCategoryBits = PlayerEntitiesCategory(player.Slot),
                    .CollideWithMaskBits =
                        CollidesWith(PlayerEntitiesCategory(sender.Slot) | Category::Base),
                    .IsTrigger = false,
                    .Offset    = Transform3D{},
                    .ShapeParams =
                        BoxShapeParams{
                            .HalfExtents = glm::vec3(0.35f, 0.65f, 0.35f),
                        },
                },
        };
        body.Body       = phy.CreateRigidBody(body.Params, t);
        collider.Handle = phy.AddCollider(body.Body, collider.Params);

        phy.World()->testOverlap(body.Body, handler);

        // Clean up temporary test body
        phy.World()->destroyRigidBody(body.Body);

        if (!handler.CanBuildTower) {
            WATO_ERR(aRegistry, "Cannot build tower at {}", t.Position);
            return;  // Ghost tower stays for repositioning
        }

        // Valid placement - server will create tower and broadcast to all clients
        // Ghost tower will be destroyed by ExitPlacement
        SPDLOG_INFO("Validated tower placement at {}", t.Position);
    }

    SPDLOG_DEBUG("exiting placement mode");
    ExitPlacement(aRegistry);
}

void PlacementModeContextHandler::operator()(Registry& aRegistry, const PlacementModePayload&)
{
    SPDLOG_DEBUG("exiting placement mode");
    ExitPlacement(aRegistry);
}

void ServerContextHandler::operator()(Registry& aRegistry, BuildTowerPayload& aPayload)
{
    auto& player = aRegistry.get<Player>(FindPlayerEntity(aRegistry, CurrentPlayerID));
    auto  tower  = aRegistry.create();
    auto& phy    = aRegistry.ctx().get<Physics>();
    auto& sender = aRegistry.get<Player>(GetSenderFor(aRegistry, player.ID));

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
                .CollisionCategoryBits = PlayerEntitiesCategory(player.Slot),
                .CollideWithMaskBits =
                    CollidesWith(PlayerEntitiesCategory(sender.Slot) | Category::Base),
                .IsTrigger = false,
                .Offset    = Transform3D{},
                .ShapeParams =
                    BoxShapeParams{
                        .HalfExtents = glm::vec3(0.35f, 0.65f, 0.35f),
                    },
            },
    };

    rp3d::RigidBody* rpb = phy.CreateRigidBody(body.Params, t);
    rp3d::Collider*  rpc = phy.AddCollider(rpb, collider.Params);

    TowerBuildingHandler handler(WATO_REG_LOGGER(aRegistry));
    phy.World()->testOverlap(rpb, handler);

    phy.World()->destroyRigidBody(rpb);

    if (!handler.CanBuildTower) {
        aRegistry.destroy(tower);
        WATO_ERR(aRegistry, "tower {} at {} invalidated", tower, t.Position);
        return;
    }

    WATO_INFO(aRegistry, "tower {} at {} validated", tower, t.Position);
    aRegistry.emplace<Tower>(tower, aPayload.Tower);
    aRegistry.emplace<RigidBody>(tower, body);
    aRegistry.emplace<Collider>(tower, collider);
    aRegistry.emplace<TowerAttack>(
        tower,
        TowerAttack{
            .Range    = 30.0f,
            .FireRate = 1.0f,
        });

    aRegistry.emplace<Owner>(tower, CurrentPlayerID, player.Slot);

    // Broadcast tower creation to all clients
    aRegistry.ctx().get<ENetServer&>().BroadcastResponse(
        GetPlayerIDs(aRegistry),
        PacketType::Ack,
        aRegistry.ctx().get<GameInstance&>().Tick,
        RigidBodyUpdateResponse{
            .Params = body.Params,
            .Entity = tower,
            .Event  = RigidBodyEvent::Create,
            .InitData =
                TowerInitData{
                    .Type           = aPayload.Tower,
                    .Position       = aPayload.Position,
                    .OwnerID        = CurrentPlayerID,
                    .ColliderParams = collider.Params,
                },
        });
}

void ServerContextHandler::operator()(Registry& aRegistry, SendCreepPayload& aPayload)
{
    entt::entity nextSpawn = GetTargetSpawnFor(aRegistry, CurrentPlayerID);
    if (nextSpawn == entt::null) {
        WATO_ERR(aRegistry, "cannot find target spawn for player {}", CurrentPlayerID);
        return;
    }
    auto& player = aRegistry.get<Player>(FindPlayerEntity(aRegistry, CurrentPlayerID));

    auto& spawnTransform = aRegistry.get<Transform3D>(nextSpawn);
    auto& spawnOwner     = aRegistry.get<Owner>(nextSpawn);

    auto& graphMap = aRegistry.ctx().get<PlayerGraphMap>();
    auto  it       = graphMap.find(spawnOwner.ID);

    if (it == graphMap.end()) {
        WATO_ERR(aRegistry, "cannot find graph for target player {}", spawnOwner.ID);
        return;
    }
    const auto& graph = it->second;

    auto creep = aRegistry.create();
    aRegistry.emplace<Transform3D>(
        creep,
        spawnTransform.Position,
        glm::identity<glm::quat>(),
        glm::vec3(0.5f));
    aRegistry.emplace<Health>(creep, 100.0f);
    aRegistry.emplace<Creep>(creep, aPayload.Type, 1.0f);
    aRegistry.emplace<Path>(
        creep,
        graph.CellFromWorld(spawnTransform.Position),
        graph.GetNextCell(spawnTransform.Position));

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
    auto& collider = aRegistry.emplace<Collider>(
        creep,
        Collider{
            .Params =
                ColliderParams{
                    .CollisionCategoryBits = PlayerEntitiesCategory(player.Slot),
                    .CollideWithMaskBits   = CollidesWith(
                        Category::Projectiles,
                        PlayerEntitiesCategory(player.Slot),
                        Category::Base),
                    .IsTrigger = false,
                    .Offset    = Transform3D{},
                    .ShapeParams =
                        CapsuleShapeParams{
                            .Radius = 0.1f,
                            .Height = 0.05f,
                        },
                },
        });

    aRegistry.emplace<Owner>(creep, CurrentPlayerID, player.Slot);
    aRegistry.emplace<Target>(creep, spawnOwner.ID, spawnOwner.Slot);

    // Broadcast creep creation to all clients
    auto& rigidBody = aRegistry.get<RigidBody>(creep);
    auto& transform = aRegistry.get<Transform3D>(creep);
    aRegistry.ctx().get<ENetServer&>().BroadcastResponse(
        GetPlayerIDs(aRegistry),
        PacketType::Ack,
        aRegistry.ctx().get<GameInstance&>().Tick,
        RigidBodyUpdateResponse{
            .Params = rigidBody.Params,
            .Entity = creep,
            .Event  = RigidBodyEvent::Create,
            .InitData =
                CreepInitData{
                    .Type           = aPayload.Type,
                    .Position       = transform.Position,
                    .OwnerID        = CurrentPlayerID,
                    .ColliderParams = collider.Params,
                },
        });
}

void ActionSystem::HandleAction(Registry& aRegistry, Action& aAction, const float aDeltaTime)
{
    WATO_TRACE(aRegistry, "handling {}", aAction);
    auto&       contextStack = aRegistry.ctx().get<ActionContextStack&>();
    const auto& currentCtx   = contextStack.front();

    switch (currentCtx.State) {
        case ActionContext::State::Default: {
            DefaultContextHandler handler;
            HandleContext(aRegistry, aAction, handler, aDeltaTime);
            break;
        }
        case ActionContext::State::Placement: {
            PlacementModeContextHandler handler;
            HandleContext(aRegistry, aAction, handler, aDeltaTime);
            break;
        }
        case ActionContext::State::Server: {
            ServerContextHandler handler;
            HandleContext(aRegistry, aAction, handler, aDeltaTime);
            break;
        }
    }
}

void ActionSystem::ProcessActions(Registry& aRegistry, ActionTag aFilterTag, const float aDeltaTime)
{
    auto&        buf           = aRegistry.ctx().get<GameStateBuffer&>();
    ActionsType& latestActions = buf.Latest().Actions;

    if (!latestActions.empty()) {
        WATO_TRACE(aRegistry, "processing {} actions", latestActions.size());
    }

    for (Action& action : latestActions) {
        if (action.Tag != aFilterTag) {
            continue;
        }
        HandleAction(aRegistry, action, aDeltaTime);
    }
}

struct ActionPayloadVisitor {
    Registry*             Reg;
    ActionContextHandler* Handler;
    float                 DeltaTime;

    void operator()(const PlacementModePayload& aPayload) const { (*Handler)(*Reg, aPayload); }
    void operator()(const MovePayload& aPayload) const { (*Handler)(*Reg, aPayload, DeltaTime); }
    void operator()(BuildTowerPayload& aPayload) const { (*Handler)(*Reg, aPayload); }
    void operator()(SendCreepPayload& aPayload) const { (*Handler)(*Reg, aPayload); }
};

void ActionSystem::HandleContext(
    Registry&             aRegistry,
    Action&               aAction,
    ActionContextHandler& aCtxHandler,
    const float           aDeltaTime)
{
    std::visit(ActionPayloadVisitor{&aRegistry, &aCtxHandler, aDeltaTime}, aAction.Payload);
}

void ServerActionSystem::Execute(Registry& aRegistry, [[maybe_unused]] std::uint32_t aTick)
{
    constexpr float kTimeStep = 1.0f / 60.0f;

    auto& taggedActions = aRegistry.ctx().get<TaggedActionsType>();

    ServerContextHandler handler;

    for (auto& [playerID, action] : taggedActions) {
        if (action.Tag != ActionTag::FixedTime) {
            continue;
        }
        handler.CurrentPlayerID = playerID;
        HandleContext(aRegistry, action, handler, kTimeStep);
    }
    taggedActions.clear();
}
