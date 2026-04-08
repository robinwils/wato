#include "systems/action.hpp"

#include <variant>

#include "components/camera.hpp"
#include "components/creep.hpp"
#include "components/game.hpp"
#include "components/health.hpp"
#include "components/net.hpp"
#include "components/path.hpp"
#include "components/placement_mode.hpp"
#include "components/rigid_body.hpp"
#include "components/spawner.hpp"
#include "components/tower.hpp"
#include "components/tower_attack.hpp"
#include "components/transform3d.hpp"
#include "core/gameplay_definitions.hpp"
#include "core/graph.hpp"
#include "core/net/enet_server.hpp"
#include "core/physics/physics.hpp"
#include "core/state.hpp"
#include "core/sys/log.hpp"
#include "core/tower_building_handler.hpp"
#include "input/action.hpp"
#include "registry/registry.hpp"

void moveCamera(Registry& aRegistry, const MovePayload& aPayload, float aDeltaTime)
{
    WATO_TRACE(aRegistry, "moving camera {}", aDeltaTime);
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

bool clientValidateTower(Registry& aRegistry, const BuildTowerPayload& aPayload)
{
    auto&       phy   = GetSingletonComponent<Physics>(aRegistry);
    const auto& graph = GetSingletonComponent<Graph>(aRegistry);

    for (const auto&& [tower, pm, t] : aRegistry.view<PlacementMode, Transform3D>().each()) {
        if (!graph.IsInside(t.Position)) {
            WATO_ERR(aRegistry, "trying to place tower outside map bounds.");
            continue;
        }

        if (!CanPlaceTower(
                phy,
                t.Position,
                TowerDef::kColliderParams,
                WATO_REG_LOGGER(aRegistry))) {
            WATO_ERR(aRegistry, "Cannot build tower at {}", t.Position);
            return false;
        }

        aRegistry.destroy(tower);
        SPDLOG_INFO("Validated tower placement at {}", t.Position);
    }

    return true;
}

void serverBuildTower(Registry& aRegistry, BuildTowerPayload& aPayload, PlayerID aPlayerID)
{
    auto& graphMap = GetSingletonComponent<PlayerGraphMap>(aRegistry);
    auto  it       = graphMap.find(aPlayerID);

    const auto& defs       = aRegistry.ctx().get<const GameplayDef&>();
    const auto  towerDefIt = defs.Towers.find(aPayload.Tower);

    if (it == graphMap.end()) {
        WATO_ERR(aRegistry, "cannot find graph for target player {}", aPlayerID);
        return;
    }
    const auto& graph = it->second;
    if (!graph.IsInside(aPayload.Position)) {
        WATO_ERR(aRegistry, "trying to place tower outside map bounds.");
        return;
    }

    if (towerDefIt == defs.Towers.end()) {
        WATO_ERR(aRegistry, "unknown tower type {}", TowerTypeToString(aPayload.Tower));
        return;
    }
    const auto& towerDef = towerDefIt->second;

    auto& player = aRegistry.get<Player>(FindPlayerEntity(aRegistry, aPlayerID));
    auto  tower  = aRegistry.create();
    auto& phy    = GetSingletonComponent<Physics>(aRegistry);
    auto& t3D    = aRegistry.emplace<Transform3D>(tower, towerDef.Transform.ToTransform3D());

    t3D.Position = aPayload.Position;

    ColliderParams colliderParams = TowerDef::kColliderParams;

    if (!CanPlaceTower(phy, t3D.Position, colliderParams, WATO_REG_LOGGER(aRegistry))) {
        aRegistry.destroy(tower);
        WATO_ERR(aRegistry, "tower {} at {} invalidated", tower, t3D.Position);
        return;
    }

    auto body = RigidBody{
        .Params = TowerDef::kRigidBodyParams,
    };
    auto collider = Collider{.Params = colliderParams};

    body.Body       = phy.CreateRigidBody(body.Params, t3D);
    collider.Handle = phy.AddCollider(body.Body, colliderParams);

    WATO_INFO(aRegistry, "tower {} at {} validated", tower, t3D.Position);
    aRegistry.emplace<Tower>(tower, aPayload.Tower);
    aRegistry.emplace<RigidBody>(tower, body);
    aRegistry.emplace<Collider>(tower, collider);
    aRegistry.emplace<TowerAttack>(tower, towerDef.Attack);
    aRegistry.emplace<Health>(tower, towerDef.Health);

    aRegistry.emplace<Owner>(tower, aPlayerID, player.Slot);

    if (auto* server = aRegistry.ctx().find<ENetServer>()) {
        server->BroadcastResponse(
            GetPlayerIDs(aRegistry),
            PacketType::Ack,
            GetSingletonComponent<GameInstance&>(aRegistry).Tick,
            RigidBodyUpdateResponse{
                .Params = body.Params,
                .Entity = tower,
                .Event  = RigidBodyEvent::Create,
                .InitData =
                    TowerInitData{
                        .Type           = aPayload.Tower,
                        .Position       = aPayload.Position,
                        .Health         = towerDef.Health,
                        .Attack         = towerDef.Attack,
                        .OwnerID        = aPlayerID,
                        .ColliderParams = collider.Params,
                    },
            });
    }
}

void serverSendCreep(Registry& aRegistry, SendCreepPayload& aPayload, PlayerID aPlayerID)
{
    entt::entity nextSpawn = GetTargetSpawnFor(aRegistry, aPlayerID);
    if (nextSpawn == entt::null) {
        WATO_ERR(aRegistry, "cannot find target spawn for player {}", aPlayerID);
        return;
    }
    const auto& creepDef = GetCreepDef(aRegistry, aPayload.Type);

    auto& player = aRegistry.get<Player>(FindPlayerEntity(aRegistry, aPlayerID));

    auto& spawnTransform = aRegistry.get<Transform3D>(nextSpawn);
    auto& spawnOwner     = aRegistry.get<Owner>(nextSpawn);

    auto& graphMap = GetSingletonComponent<PlayerGraphMap>(aRegistry);
    auto  it       = graphMap.find(spawnOwner.ID);

    if (it == graphMap.end()) {
        WATO_ERR(aRegistry, "cannot find graph for target player {}", spawnOwner.ID);
        return;
    }
    const auto& graph = it->second;

    auto  creep  = aRegistry.create();
    auto& t3D    = aRegistry.emplace<Transform3D>(creep, creepDef.Transform.ToTransform3D());
    t3D.Position = spawnTransform.Position;

    aRegistry.emplace<Health>(creep, creepDef.Health);
    aRegistry.emplace<Creep>(creep, aPayload.Type, creepDef.Damage);
    aRegistry.emplace<Path>(
        creep,
        graph.CellFromWorld(spawnTransform.Position),
        graph.GetNextCell(spawnTransform.Position));

    auto& body = aRegistry.emplace<RigidBody>(
        creep,
        RigidBody{
            .Params = CreepDef::kRigidBodyParams,
        });
    body.Params.Velocity = creepDef.Speed;

    auto& collider = aRegistry.emplace<Collider>(
        creep,
        Collider{
            .Params = CreepDef::kColliderParams,
        });
    collider.Params.CollisionCategoryBits = PlayerEntitiesCategory(player.Slot);
    collider.Params.CollideWithMaskBits =
        CollidesWith(Category::Projectiles, PlayerEntitiesCategory(player.Slot), Category::Base);

    aRegistry.emplace<Owner>(creep, aPlayerID, player.Slot);
    aRegistry.emplace<Target>(creep, spawnOwner.ID, spawnOwner.Slot);

    if (auto* server = aRegistry.ctx().find<ENetServer>()) {
        server->BroadcastResponse(
            GetPlayerIDs(aRegistry),
            PacketType::Ack,
            GetSingletonComponent<GameInstance&>(aRegistry).Tick,
            RigidBodyUpdateResponse{
                .Params = body.Params,
                .Entity = creep,
                .Event  = RigidBodyEvent::Create,
                .InitData =
                    CreepInitData{
                        .Type           = aPayload.Type,
                        .Position       = t3D.Position,
                        .Health         = creepDef.Health,
                        .Damage         = creepDef.Damage,
                        .OwnerID        = aPlayerID,
                        .ColliderParams = collider.Params,
                    },
            });
    }
}

void RealTimeActionSystem::Execute(Registry& aRegistry, float aDelta)
{
    auto& buf   = GetSingletonComponent<FrameActionBuffer&>(aRegistry);
    auto& stack = GetSingletonComponent<ActionContextStack&>(aRegistry);

    for (Action& action : buf) {
        if (auto* move = std::get_if<MovePayload>(&action.Payload)) {
            moveCamera(aRegistry, *move, aDelta);
        } else if (auto* placement = std::get_if<PlacementModePayload>(&action.Payload)) {
            stack.TogglePlacement(aRegistry, *placement);
        } else {
            WATO_TRACE(aRegistry, "unhandled frame-time action: {}", action);
        }
    }
    buf.clear();
}

void DeterministicActionSystem::Execute(Registry& aRegistry, [[maybe_unused]] std::uint32_t aTick)
{
    auto& buf   = GetSingletonComponent<GameStateBuffer&>(aRegistry);
    auto& stack = GetSingletonComponent<ActionContextStack&>(aRegistry);

    for (Action& action : buf.Latest().Actions) {
        if (auto* p = std::get_if<BuildTowerPayload>(&action.Payload)) {
            if (stack.GetState<PlacementState>()) {
                if (clientValidateTower(aRegistry, *p)) {
                    stack.ExitPlacement(aRegistry);
                }
            }
        } else if (std::get_if<SendCreepPayload>(&action.Payload)) {
            // client no-op, server handles
        } else {
            WATO_TRACE(aRegistry, "unhandled fixed-time action: {}", action);
        }
    }
}

void ServerActionSystem::Execute(Registry& aRegistry, [[maybe_unused]] std::uint32_t aTick)
{
    auto& taggedActions = GetSingletonComponent<TaggedActionsType>(aRegistry);

    for (auto& [playerID, action] : taggedActions) {
        if (auto* build = std::get_if<BuildTowerPayload>(&action.Payload)) {
            serverBuildTower(aRegistry, *build, playerID);
        } else if (auto* creep = std::get_if<SendCreepPayload>(&action.Payload)) {
            serverSendCreep(aRegistry, *creep, playerID);
        } else {
            WATO_TRACE(aRegistry, "unhandled server action: {}", action);
        }
    }
    taggedActions.clear();
}
