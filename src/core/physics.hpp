#pragma once

#include <fmt/base.h>

#include <entt/entity/fwd.hpp>
#include <glm/glm.hpp>

#include "components/rigid_body.hpp"
#include "components/transform3d.hpp"
#include "config.h"
#include "reactphysics3d/reactphysics3d.h"

struct PhysicsParams {
    bool InfoLogs;
    bool WarningLogs;
    bool ErrorLogs;

    reactphysics3d::DefaultLogger* Logger;

#if WATO_DEBUG
    bool RenderShapes;
    bool RenderAabb = true;
    bool RenderContactPoints;
    bool RenderContactNormals;
#endif
};

struct RigidBodyParams {
    rp3d::BodyType  Type{rp3d::BodyType::STATIC};
    rp3d::Transform Transform{rp3d::Transform::identity()};
    bool            GravityEnabled{true};
};

class Physics
{
   public:
    Physics() {}
    Physics(const Physics&) = delete;
    Physics(Physics&& aPhy) : mWorld(aPhy.mWorld) {}
    Physics& operator=(const Physics&) = delete;
    Physics& operator=(Physics&&)      = delete;

    void                               Init(Registry& aRegistry);
    void                               InitLogger();
    [[nodiscard]] rp3d::PhysicsWorld*  World() noexcept { return mWorld; }
    [[nodiscard]] rp3d::PhysicsWorld*  World() const noexcept { return mWorld; }
    [[nodiscard]] rp3d::PhysicsCommon& Common() noexcept { return mCommon; }

    void test()
    {
        for (uint32_t rbIdx = 0; rbIdx < mWorld->getNbRigidBodies(); ++rbIdx) {
            const rp3d::RigidBody*  rb          = mWorld->getRigidBody(rbIdx);
            const rp3d::Transform   transform   = rb->getTransform();
            const rp3d::Vector3&    position    = transform.getPosition();
            const rp3d::Quaternion& orientation = transform.getOrientation();
        }
    }
    constexpr static auto Serialize(auto& aArchive, const auto& aSelf)
    {
        uint32_t nbRigidBodies = aSelf.World()->getNbRigidBodies();
        aArchive.template Write<uint32_t>(&nbRigidBodies, 1);
        for (uint32_t rbIdx = 0; rbIdx < nbRigidBodies; ++rbIdx) {
            const rp3d::RigidBody*  rb          = aSelf.World()->getRigidBody(rbIdx);
            const rp3d::Transform&  transform   = rb->getTransform();
            const rp3d::Vector3&    position    = transform.getPosition();
            const rp3d::Quaternion& orientation = transform.getOrientation();
    rp3d::RigidBody* CreateRigidBody(const entt::entity& aEntity,
        Registry&                                        aRegistry,
        const RigidBodyParams                            aParams);
    rp3d::Collider*  AddBoxCollider(rp3d::RigidBody* aBody,
         const rp3d::Vector3&                        aSize,
         const bool                                  aIsTrigger = false);
    void             DeleteRigidBody(Registry& aRegistry, entt::entity aEntity);

            aArchive.template Write<float>(&position.x, 3);
            aArchive.template Write<float>(&orientation.x, 4);
        }
    }

    static auto Deserialize(auto& aArchive, auto& aSelf)
    {
        aSelf.Init();
        uint32_t nbRigidBodies = 0;
        aArchive.template Read<uint32_t>(&nbRigidBodies, 3);
        for (uint32_t rbIdx = 0; rbIdx < nbRigidBodies; ++rbIdx) {
            rp3d::Vector3    position;
            rp3d::Quaternion orientation;
            aArchive.template Read<float>(&position.x, 3);
            aArchive.template Read<float>(&orientation.x, 4);

            fmt::println("creating rigid body for entity {:d}", static_cast<ENTT_ID_TYPE>(entity));
            rp3d::RigidBody* body = phy.CreateRigidBody(entity,
                aRegistry,
                RigidBodyParams{.Type = type,
                    .Transform        = rp3d::Transform(position, orientation),
                    .GravityEnabled   = gravity});
            aRegistry.template emplace<RigidBody>(entity, body);
        }
    }

    PhysicsParams Params;

   private:
    rp3d::PhysicsCommon mCommon;
    rp3d::PhysicsWorld* mWorld = nullptr;
};
