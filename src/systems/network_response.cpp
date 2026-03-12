#include "systems/network_response.hpp"

#include <entt/core/fwd.hpp>

#include "components/animator.hpp"
#include "components/creep.hpp"
#include "components/health.hpp"
#include "components/imgui.hpp"
#include "components/model_rotation_offset.hpp"
#include "components/player.hpp"
#include "components/projectile.hpp"
#include "components/rigid_body.hpp"
#include "components/scene_object.hpp"
#include "components/tower.hpp"
#include "components/tower_attack.hpp"
#include "components/transform3d.hpp"
#include "core/physics/physics.hpp"
#include "core/snapshot.hpp"
#include "core/sys/log.hpp"
#include "core/types.hpp"
#include "registry/registry.hpp"

using namespace entt::literals;

void NetworkResponseSystem::ensureConnected(entt::dispatcher& aDispatcher)
{
    if (mConnected) {
        return;
    }

    aDispatcher.sink<RigidBodyUpdateEvent>().connect<&NetworkResponseSystem::onRigidBodyUpdate>(
        *this);
    aDispatcher.sink<HealthUpdateEvent>().connect<&NetworkResponseSystem::onHealthUpdate>(*this);
    aDispatcher.sink<SyncPayloadEvent>().connect<&NetworkResponseSystem::onSyncPayload>(*this);

    mConnected = true;
}

void NetworkResponseSystem::Execute(Registry& aRegistry, [[maybe_unused]] std::uint32_t aTick)
{
    auto& dispatcher = aRegistry.ctx().get<entt::dispatcher>("net_dispatcher"_hs);

    ensureConnected(dispatcher);
    dispatcher.update();
}
void NetworkResponseSystem::onRigidBodyUpdate(const RigidBodyUpdateEvent& aEvent)
{
    Registry&   registry = *aEvent.Reg;
    const auto& update   = aEvent.Response;
    auto&       syncMap  = GetSingletonComponent<EntitySyncMap>(registry);

    struct EntityCreateVisitor {
        NetworkResponseSystem*         Self;
        Registry*                      Reg;
        const RigidBodyUpdateResponse* Update;

        void operator()(const ProjectileInitData& aInit) const
        {
            Self->createProjectile(*Reg, *Update, aInit);
        }
        void operator()(const TowerInitData& aInit) const
        {
            Self->createTower(*Reg, *Update, aInit);
        }
        void operator()(const CreepInitData& aInit) const
        {
            Self->createCreep(*Reg, *Update, aInit);
        }
        void operator()(const std::monostate&) const
        {
            WATO_INFO(*Reg, "created entity {} (no specific init data)", Update->Entity);
        }
    };

    switch (update.Event) {
        case RigidBodyEvent::Create: {
            std::visit(EntityCreateVisitor{this, &registry, &update}, update.InitData);
            break;
        }
        case RigidBodyEvent::Update: {
            if (syncMap.contains(update.Entity)) {
                registry.patch<RigidBody>(syncMap[update.Entity], [&update](RigidBody& aBody) {
                    aBody.Params = update.Params;
                });
            }
            break;
        }
        case RigidBodyEvent::Destroy: {
            if (syncMap.contains(update.Entity)) {
                entt::entity clientEntity = syncMap[update.Entity];
                registry.destroy(clientEntity);
                syncMap.erase(update.Entity);
                WATO_INFO(
                    registry,
                    "destroyed entity {} (server entity {})",
                    clientEntity,
                    update.Entity);
            }
            break;
        }
    }
}

void NetworkResponseSystem::onHealthUpdate(const HealthUpdateEvent& aEvent)
{
    Registry&   registry = *aEvent.Reg;
    const auto& update   = aEvent.Response;
    auto&       syncMap  = GetSingletonComponent<EntitySyncMap>(registry);

    if (syncMap.contains(update.Entity)) {
        entt::entity e = syncMap[update.Entity];
        WATO_INFO(
            registry,
            "received health update for server {}, updating local {} => {}",
            update.Entity,
            e,
            update.Health);
        registry.patch<Health>(e, [&update](Health& aHealth) { aHealth.Health = update.Health; });
    } else {
        WATO_WARN(
            registry,
            "received health update for server {} with {}, but entity not synced",
            update.Entity,
            update.Health);
    }
}

void NetworkResponseSystem::onSyncPayload(const SyncPayloadEvent& aEvent)
{
    Registry&   registry = *aEvent.Reg;
    const auto& payload  = aEvent.Payload;

    if (payload.State.Snapshot.empty()) {
        return;
    }

    BitInputArchive       inAr(payload.State.Snapshot);
    Registry              tmp;
    entt::snapshot_loader loader{tmp};

    WATO_TRACE(
        registry,
        "loading state snapshot {} of size {}",
        payload.State.Tick,
        payload.State.Snapshot.size());

    loader.get<entt::entity>(inAr).get<Transform3D>(inAr).get<RigidBody>(inAr).get<Collider>(inAr);
}

void NetworkResponseSystem::createProjectile(
    Registry&                      aRegistry,
    const RigidBodyUpdateResponse& aUpdate,
    const ProjectileInitData&      aInit)
{
    auto& syncMap = GetSingletonComponent<EntitySyncMap>(aRegistry);

    WATO_INFO(aRegistry, "got projectile init data");

    auto sourceTowerIt = syncMap.find(aInit.SourceTower);
    if (sourceTowerIt == syncMap.end()) {
        WATO_WARN(aRegistry, "source tower {} not found in sync map", aInit.SourceTower);
        return;
    }

    entt::entity clientTower    = sourceTowerIt->second;
    auto*        towerTransform = aRegistry.try_get<Transform3D>(clientTower);
    if (!towerTransform) {
        WATO_WARN(aRegistry, "source tower {} has no transform", clientTower);
        return;
    }

    auto projectile = aRegistry.create();

    glm::vec3 position = towerTransform->Position + glm::vec3(0.0f, 0.5f, 0.0f);

    aRegistry
        .emplace<Transform3D>(projectile, position, glm::identity<glm::quat>(), glm::vec3(0.05f));

    aRegistry.emplace<ModelRotationOffset>(
        projectile,
        glm::angleAxis(glm::radians(180.0f), glm::vec3(0, 1, 0)));

    entt::entity clientTarget = entt::null;
    auto         targetIt     = syncMap.find(aInit.Target);
    if (targetIt != syncMap.end()) {
        clientTarget = targetIt->second;
    } else {
        WATO_ERR(aRegistry, "projectile server target {} unknown", aInit.Target);
    }

    aRegistry.emplace<Projectile>(projectile, aInit.Damage, aInit.Speed, clientTarget);
    aRegistry.emplace<RigidBody>(projectile, RigidBody{.Params = aUpdate.Params});
    aRegistry.emplace<Collider>(projectile, aInit.ColliderParams);
    aRegistry.emplace<SceneObject>(projectile, "arrow"_hs);

    syncMap.insert_or_assign(aUpdate.Entity, projectile);

    WATO_INFO(aRegistry, "created projectile {} from server entity {}", projectile, aUpdate.Entity);
}

void NetworkResponseSystem::createTower(
    Registry&                      aRegistry,
    const RigidBodyUpdateResponse& aUpdate,
    const TowerInitData&           aInit)
{
    auto&       syncMap = GetSingletonComponent<EntitySyncMap>(aRegistry);
    auto&       phy     = GetSingletonComponent<Physics>(aRegistry);
    auto&       player  = aRegistry.get<Player>(FindPlayerEntity(aRegistry, aInit.OwnerID));
    auto&       sender  = aRegistry.get<Player>(GetSenderFor(aRegistry, aInit.OwnerID));
    const auto& def     = GetTowerDef(aRegistry, aInit.Type);

    auto tower = aRegistry.create();

    auto& transform    = aRegistry.emplace<Transform3D>(tower, def.Transform.ToTransform3D());
    transform.Position = aInit.Position;

    Collider collider{
        .Params = aInit.ColliderParams,
    };
    rp3d::RigidBody* body = phy.CreateRigidBody(aUpdate.Params, transform);
    rp3d::Collider*  c    = phy.AddCollider(body, collider.Params);

    aRegistry.emplace<Tower>(tower, aInit.Type);
    aRegistry.emplace<RigidBody>(tower, aUpdate.Params, body);
    aRegistry.emplace<Collider>(tower, collider.Params, c);
    aRegistry.emplace<Health>(tower, aInit.Health);
    aRegistry.emplace<SceneObject>(tower, def.Model.Object);
    aRegistry.emplace<Owner>(tower, aInit.OwnerID, player.Slot);
    aRegistry.emplace<TowerAttack>(tower, aInit.Attack);

    syncMap.insert_or_assign(aUpdate.Entity, tower);

    WATO_INFO(aRegistry, "created tower {} from server entity {}", tower, aUpdate.Entity);
}

void NetworkResponseSystem::createCreep(
    Registry&                      aRegistry,
    const RigidBodyUpdateResponse& aUpdate,
    const CreepInitData&           aInit)
{
    auto& syncMap = GetSingletonComponent<EntitySyncMap>(aRegistry);
    auto& phy     = GetSingletonComponent<Physics>(aRegistry);
    auto& player  = aRegistry.get<Player>(FindPlayerEntity(aRegistry, aInit.OwnerID));

    const auto& creepDef = GetCreepDef(aRegistry, aInit.Type);

    auto creep = aRegistry.create();

    auto& transform = aRegistry.emplace<Transform3D>(
        creep,
        aInit.Position,
        glm::identity<glm::quat>(),
        glm::vec3(0.5f));

    aRegistry.emplace<ModelRotationOffset>(
        creep,
        glm::angleAxis(glm::radians(90.0f), glm::vec3(0, 1, 0)));

    RigidBody body{.Params = aUpdate.Params};
    Collider  collider{
         .Params = aInit.ColliderParams,
    };
    body.Body       = phy.CreateRigidBody(body.Params, transform);
    collider.Handle = phy.AddCollider(body.Body, collider.Params);

    aRegistry.emplace<Creep>(creep, aInit.Type);
    aRegistry.emplace<RigidBody>(creep, body);
    aRegistry.emplace<Collider>(creep, collider);
    aRegistry.emplace<Health>(creep, aInit.Health);
    aRegistry.emplace<Owner>(creep, aInit.OwnerID, player.Slot);
    aRegistry.emplace<SceneObject>(creep, creepDef.Model.Object);
    aRegistry.emplace<ImguiDrawable>(creep, creepDef.Model.Object.Name, true);
    aRegistry.emplace<Animator>(creep, creepDef.Model.Animation);

    syncMap.insert_or_assign(aUpdate.Entity, creep);

    WATO_INFO(aRegistry, "created creep {} from server entity {}", creep, aUpdate.Entity);
}
