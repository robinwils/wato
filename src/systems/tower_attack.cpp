#include "systems/tower_attack.hpp"

#include <glm/geometric.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>
#include <limits>

#include "components/creep.hpp"
#include "components/game.hpp"
#include "components/health.hpp"
#include "components/projectile.hpp"
#include "components/rigid_body.hpp"
#include "components/tower.hpp"
#include "components/tower_attack.hpp"
#include "components/transform3d.hpp"
#include "core/net/enet_server.hpp"
#include "core/net/net.hpp"
#include "core/physics/physics.hpp"
#include "core/sys/log.hpp"

// Callback for collecting colliders within range using sphere overlap query
class ColliderCollectorCallback : public rp3d::OverlapCallback
{
   public:
    void onOverlap(CallbackData& aData) override
    {
        for (uint32_t i = 0; i < aData.getNbOverlappingPairs(); ++i) {
            auto pair = aData.getOverlappingPair(i);

            // Get the collider from the overlap (skip collider1 as it's our query sphere)
            auto* collider = pair.getCollider2();

            // Only collect entities with the Entities category
            if (collider->getCollisionCategoryBits() == Category::Entities) {
                mColliders.push_back(collider);
            }
        }
    }

    const std::vector<rp3d::Collider*>& GetColliders() const { return mColliders; }

   private:
    std::vector<rp3d::Collider*> mColliders;
};

void TowerAttackSystem::operator()(Registry& aRegistry, float aDeltaTime)
{
    auto& phy = aRegistry.ctx().get<Physics>();

    for (auto&& [towerEntity, tower, towerTransform, attack] :
         aRegistry.view<Tower, Transform3D, TowerAttack>().each()) {
        attack.TimeSinceLastShot += aDeltaTime;

        bool needNewTarget = true;
        if (aRegistry.valid(attack.CurrentTarget)) {
            auto* targetTransform = aRegistry.try_get<Transform3D>(attack.CurrentTarget);
            auto* targetHealth    = aRegistry.try_get<Health>(attack.CurrentTarget);

            if (targetTransform && targetHealth && targetHealth->Health > 0.0f) {
                float distance = glm::distance(towerTransform.Position, targetTransform->Position);

                if (distance <= attack.Range) {
                    needNewTarget = false;
                }
            }
        }

        if (needNewTarget) {
            attack.CurrentTarget = entt::null;

            // Create a temporary kinematic body with sphere collider for spatial query
            rp3d::Transform queryTransform(
                ToRP3D(towerTransform.Position),
                rp3d::Quaternion::identity());
            rp3d::RigidBody* queryBody = phy.World()->createRigidBody(queryTransform);
            queryBody->setType(rp3d::BodyType::KINEMATIC);

            // Create sphere shape matching tower's attack range
            rp3d::SphereShape* sphereShape = phy.Common().createSphereShape(attack.Range);

            rp3d::Collider* queryCollider =
                queryBody->addCollider(sphereShape, rp3d::Transform::identity());
            queryCollider->setCollisionCategoryBits(Category::Projectiles);
            queryCollider->setCollideWithMaskBits(Category::Entities);
            queryCollider->setIsTrigger(true);

            ColliderCollectorCallback callback;
            phy.World()->testOverlap(queryBody, callback);

            phy.World()->destroyRigidBody(queryBody);
            phy.Common().destroySphereShape(sphereShape);

            auto&        colliderToEntity = aRegistry.ctx().get<ColliderEntityMap>();
            float        closestDistSq    = std::numeric_limits<float>::max();
            entt::entity closestTarget    = entt::null;

            for (auto* collider : callback.GetColliders()) {
                auto it = colliderToEntity.find(collider);
                if (it == colliderToEntity.end()) {
                    continue;
                }

                entt::entity entity = it->second;

                auto* creep     = aRegistry.try_get<Creep>(entity);
                auto* health    = aRegistry.try_get<Health>(entity);
                auto* transform = aRegistry.try_get<Transform3D>(entity);

                if (!creep || !health || !transform || health->Health <= 0.0f) {
                    continue;
                }

                float distSq = glm::distance2(towerTransform.Position, transform->Position);

                if (distSq < closestDistSq) {
                    closestDistSq = distSq;
                    closestTarget = entity;
                }
            }

            attack.CurrentTarget = closestTarget;
        }

        if (aRegistry.valid(attack.CurrentTarget)
            && attack.TimeSinceLastShot >= 1.0f / attack.FireRate) {
            attack.TimeSinceLastShot = 0.0f;

            auto  projectile      = aRegistry.create();
            auto* targetTransform = aRegistry.try_get<Transform3D>(attack.CurrentTarget);

            auto pT = aRegistry.emplace<Transform3D>(
                projectile,
                towerTransform.Position + glm::vec3(0.0f, 0.5f, 0.0f),
                glm::identity<glm::quat>(),
                glm::vec3(0.1f));

            aRegistry.emplace<Projectile>(projectile, 10.0f, 1.0f, attack.CurrentTarget);

            glm::vec3 direction = glm::normalize(targetTransform->Position - pT.Position);

            aRegistry.emplace<RigidBody>(
                projectile,
                RigidBody{
                    .Params =
                        RigidBodyParams{
                            .Type           = rp3d::BodyType::KINEMATIC,
                            .Velocity       = 5.0f,
                            .Direction      = direction,
                            .GravityEnabled = false,
                        },
                });

            aRegistry.emplace<Collider>(
                projectile,
                Collider{
                    .Params =
                        ColliderParams{
                            .CollisionCategoryBits = Category::Projectiles,
                            .CollideWithMaskBits   = Category::Entities,
                            .IsTrigger             = true,
                            .Offset                = Transform3D{},
                            .ShapeParams =
                                CapsuleShapeParams{
                                    .Radius = 0.05f,
                                    .Height = 0.1f,
                                },
                        },
                });

            WATO_DBG(
                aRegistry,
                "tower {} fired projectile {} at target {}",
                towerEntity,
                projectile,
                attack.CurrentTarget);
        }
    }
}
