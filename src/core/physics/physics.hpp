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

struct BoxShapeParams {
    glm::vec3 HalfExtents;

    bool Archive(auto& aArchive)
    {
        if (!ArchiveVector(aArchive, HalfExtents, 0.0f, 10.0f)) return false;
        return true;
    }
};

struct CapsuleShapeParams {
    float Radius;
    float Height;

    bool Archive(auto& aArchive)
    {
        if (!ArchiveValue(aArchive, Radius, 0.0f, 10.0f)) return false;
        if (!ArchiveValue(aArchive, Height, 0.0f, 10.0f)) return false;
        return true;
    }
};

struct HeightFieldShapeParams {
    std::vector<float> Data{};
    int                Rows;
    int                Columns;

    bool Archive(auto& aArchive)
    {
        if (!ArchiveValue(aArchive, Rows, 0, 1000)) return false;
        if (!ArchiveValue(aArchive, Columns, 0, 1000)) return false;
        return true;
    }
};

using ColliderShapeParams =
    std::variant<BoxShapeParams, CapsuleShapeParams, HeightFieldShapeParams>;

struct RigidBodyParams {
    rp3d::BodyType Type{rp3d::BodyType::STATIC};
    float          Velocity;
    glm::vec3      Direction;
    bool           GravityEnabled{true};
    void*          Data{nullptr};

    bool Archive(auto& aArchive)
    {
        if (!ArchiveValue(aArchive, Type, 0, 3)) return false;
        if (!ArchiveValue(aArchive, Velocity, 0.0f, 100.0f)) return false;
        if (!ArchiveVector(aArchive, Direction, 0.0f, 1.0f)) return false;
        return ArchiveBool(aArchive, GravityEnabled);
    }
};

inline bool operator==(const RigidBodyParams& aLHS, const RigidBodyParams& aRHS)
{
    return aLHS.Type == aRHS.Type && aLHS.Velocity == aRHS.Velocity
           && aLHS.Direction == aRHS.Direction && aLHS.GravityEnabled == aRHS.GravityEnabled;
}

struct ColliderParams {
    unsigned short      CollisionCategoryBits;
    unsigned short      CollideWithMaskBits;
    bool                IsTrigger{false};
    Transform3D         Offset{};
    ColliderShapeParams ShapeParams;
};

using ColliderEntityMap = std::unordered_map<reactphysics3d::Collider*, entt::entity>;

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

struct RigidBodyUserData {
};

class Physics
{
   public:
    // Enumeration for categories
    enum Category {
        Terrain     = 0x0001,
        Entities    = 0x0002,
        Projectiles = 0x0004,
        Count       = (Projectiles << 1) - 1
    };

    Physics(const Logger& aLogger) : mLogger(aLogger) {}
    ~Physics() { mLogger->trace("destroying physics"); }

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

    void ToggleObstacle(const rp3d::Collider* aCollider, Graph& aGraph, bool aAdd);

    PhysicsParams Params;

   private:
    rp3d::PhysicsCommon mCommon;
    rp3d::PhysicsWorld* mWorld = nullptr;
    Logger              mLogger;
};

using Category = Physics::Category;

inline rp3d::Vector3 ToRP3D(const glm::vec3 aVector)
{
    return rp3d::Vector3(aVector.x, aVector.y, aVector.z);
}

/// Matches a collision pair against expected categories.
/// Returns colliders ordered as {aFirstCategory, aSecondCategory}.
/// Returns {nullptr, nullptr} if categories don't match.
inline std::pair<rp3d::Collider*, rp3d::Collider*> MatchColliderPair(
    rp3d::Collider* aCollider1,
    rp3d::Collider* aCollider2,
    unsigned short  aFirstCategory,
    unsigned short  aSecondCategory)
{
    auto cat1 = aCollider1->getCollisionCategoryBits();
    auto cat2 = aCollider2->getCollisionCategoryBits();

    if ((cat1 & aFirstCategory) && (cat2 & aSecondCategory)) {
        return {aCollider1, aCollider2};
    }
    if ((cat2 & aFirstCategory) && (cat1 & aSecondCategory)) {
        return {aCollider2, aCollider1};
    }
    return {nullptr, nullptr};
}

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
