#pragma once

#include <fmt/base.h>
#include <fmt/format.h>

#include <entt/entity/fwd.hpp>
#include <glm/glm.hpp>
#include <vector>

#include "config.h"
#include "core/event_handler.hpp"

// Enumeration for categories
enum Category { PlacementGhostTower = 0x0001, Terrain = 0x0002, Entities = 0x0004 };

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

    rp3d::RigidBody* CreateRigidBody(
        const entt::entity&   aEntity,
        Registry&             aRegistry,
        const RigidBodyParams aParams);
    rp3d::Collider* AddBoxCollider(
        rp3d::RigidBody*     aBody,
        const rp3d::Vector3& aSize,
        const bool           aIsTrigger = false);
    rp3d::Collider* AddCapsuleCollider(
        rp3d::RigidBody* aBody,
        const float&     aRadius,
        const float&     aHeight,
        const bool       aIsTrigger = false);
    void DeleteRigidBody(Registry& aRegistry, entt::entity aEntity);

    static constexpr auto Serialize(auto& aArchive, const auto& aSelf)
    {
        uint32_t nbRigidBodies = aSelf.World()->getNbRigidBodies();
        aArchive.template Write<uint32_t>(&nbRigidBodies, 1);
        for (uint32_t rbIdx = 0; rbIdx < nbRigidBodies; ++rbIdx) {
            rp3d::RigidBody*        body        = aSelf.World()->getRigidBody(rbIdx);
            const rp3d::Transform&  transform   = body->getTransform();
            const rp3d::Vector3&    position    = transform.getPosition();
            const rp3d::Quaternion& orientation = transform.getOrientation();
            const rp3d::BodyType    type        = body->getType();
            const bool              gravity     = body->isGravityEnabled();
            const RigidBodyData*    data        = static_cast<RigidBodyData*>(body->getUserData());

            fmt::println(
                "serializing rigid body for entity {:d}",
                static_cast<ENTT_ID_TYPE>(data->Entity));
            aArchive.template Write<rp3d::BodyType>(&type, 1);
            aArchive.template Write<float>(&position.x, 3);
            aArchive.template Write<float>(&orientation.x, 4);
            aArchive.template Write<float>(&gravity, 1);
            aArchive.template Write<entt::entity>(&data->Entity, 1);
        }
    }

    /**
     * @brief special deserialize with registry because Physics is used as a context variable
     *
     * @param aArchive snapshot archive
     * @param aRegistry entt registry
     */
    static auto Deserialize(auto& aArchive, auto& aRegistry)
    {
        auto& phy = aRegistry.ctx().template emplace<Physics>();
        phy.Init(aRegistry);

        uint32_t nbRigidBodies = 0;
        aArchive.template Read<uint32_t>(&nbRigidBodies, 1);
        for (uint32_t rbIdx = 0; rbIdx < nbRigidBodies; ++rbIdx) {
            rp3d::BodyType   type;
            rp3d::Vector3    position;
            rp3d::Quaternion orientation;
            entt::entity     entity;
            bool             gravity;

            aArchive.template Read<rp3d::BodyType>(&type, 1);
            aArchive.template Read<float>(&position.x, 3);
            aArchive.template Read<float>(&orientation.x, 4);
            aArchive.template Read<float>(&gravity, 1);
            aArchive.template Read<entt::entity>(&entity, 1);

            phy.CreateRigidBody(
                entity,
                aRegistry,
                RigidBodyParams{
                    .Type           = type,
                    .Transform      = rp3d::Transform(position, orientation),
                    .GravityEnabled = gravity});
        }
        return true;
    }

    PhysicsParams Params;

   private:
    rp3d::PhysicsCommon mCommon;
    rp3d::PhysicsWorld* mWorld = nullptr;
};

inline rp3d::Vector3 ToRP3D(const glm::vec3 aVector)
{
    return rp3d::Vector3(aVector.x, aVector.y, aVector.z);
}

struct WorldRaycastCallback : public rp3d::RaycastCallback {
    virtual rp3d::decimal notifyRaycastHit(const rp3d::RaycastInfo& aInfo) override
    {
        if (aInfo.hitFraction == 0.0f) return rp3d::decimal(-1.0f);
        Hits.push_back(glm::vec3(aInfo.worldPoint.x, aInfo.worldPoint.y, aInfo.worldPoint.z));

        // Return a fraction of 1.0 to gather all hits
        return aInfo.hitFraction;
    }

    std::string String() const
    {
        std::string res;
        for (const auto& hit : Hits) {
            res = fmt::format("{} ({:f}, {:f}, {:f})", res, hit.x, hit.y, hit.z);
        }
        return res;
    }

    std::vector<glm::vec3> Hits;
};
