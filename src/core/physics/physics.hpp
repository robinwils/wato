#pragma once

#include <reactphysics3d/reactphysics3d.h>
#include <spdlog/spdlog.h>

#include <entt/entity/fwd.hpp>
#include <glm/glm.hpp>

#include "components/transform3d.hpp"
#include "core/graph.hpp"

template <>
struct fmt::formatter<rp3d::Vector3> : fmt::formatter<std::string> {
    auto format(rp3d::Vector3 aObj, format_context& aCtx) const -> decltype(aCtx.out())
    {
        return fmt::format_to(aCtx.out(), "rp3d::Vector3({}, {}, {})", aObj.x, aObj.y, aObj.z);
    }
};

// Enumeration for categories
enum Category { PlacementGhostTower = 0x0001, Terrain = 0x0002, Entities = 0x0004 };

struct BoxShapeParams {
    glm::vec3 HalfExtents;
};

struct CapsuleShapeParams {
    float Radius;
    float Height;
};

struct HeightFieldShapeParams {
    std::vector<float> Data;
    int                Rows;
    int                Columns;
};

using ColliderShapeParams =
    std::variant<BoxShapeParams, CapsuleShapeParams, HeightFieldShapeParams>;

struct RigidBodyParams {
    rp3d::BodyType Type{rp3d::BodyType::STATIC};
    float          Velocity;
    glm::vec3      Direction;
    bool           GravityEnabled{true};
    void*          Data{nullptr};
};

struct ColliderParams {
    unsigned short      CollisionCategoryBits;
    unsigned short      CollideWithMaskBits;
    bool                IsTrigger{false};
    Transform3D         Offset{};
    ColliderShapeParams ShapeParams;
};

struct PhysicsParams {
    bool InfoLogs    = false;
    bool WarningLogs = false;
    bool ErrorLogs   = true;

    reactphysics3d::DefaultLogger* Logger = nullptr;

#if WATO_DEBUG
    bool RenderShapes         = false;
    bool RenderAabb           = true;
    bool RenderContactPoints  = false;
    bool RenderContactNormals = false;
#endif
};

class Physics
{
   public:
    Physics() {}
    ~Physics() { spdlog::trace("destroying physics"); }

    Physics(const Physics&)            = delete;
    Physics& operator=(const Physics&) = delete;
    Physics& operator=(Physics&&)      = delete;

    void                               Init();
    void                               InitLogger();
    [[nodiscard]] rp3d::PhysicsWorld*  World() noexcept { return mWorld; }
    [[nodiscard]] rp3d::PhysicsWorld*  World() const noexcept { return mWorld; }
    [[nodiscard]] rp3d::PhysicsCommon& Common() noexcept { return mCommon; }

    rp3d::RigidBody* CreateRigidBodyAndCollider(
        RigidBodyParams&   aRigidBodyParams,
        ColliderParams&    aColliderParams,
        const Transform3D& aTransform);

    rp3d::CollisionShape* CreateCollisionShape(const ColliderShapeParams& aParams);
    rp3d::Collider*       AddCollider(rp3d::RigidBody* aBody, const ColliderParams& aParams);
    rp3d::RigidBody* CreateRigidBody(const RigidBodyParams& aParams, const Transform3D& aTransform);

    std::optional<glm::vec3> RayTerrainIntersection(glm::vec3 aOrigin, glm::vec3 aEnd);

    PhysicsParams Params;

   private:
    rp3d::PhysicsCommon mCommon;
    rp3d::PhysicsWorld* mWorld = nullptr;
};

inline rp3d::Vector3 ToRP3D(const glm::vec3 aVector)
{
    return rp3d::Vector3(aVector.x, aVector.y, aVector.z);
}

void ToggleObstacle(const rp3d::Collider* aCollider, Graph& aGraph, bool aAdd);

struct WorldRaycastCallback : public rp3d::RaycastCallback {
    virtual rp3d::decimal notifyRaycastHit(const rp3d::RaycastInfo& aInfo) override
    {
        if (aInfo.hitFraction == 0.0f) return -1.0f;
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
