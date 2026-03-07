#pragma once

#include <reactphysics3d/reactphysics3d.h>

#include <glm/vec3.hpp>

#include "core/physics/physics.hpp"
#include "core/sys/log.hpp"

class TowerBuildingHandler : public rp3d::OverlapCallback
{
   public:
    TowerBuildingHandler(const Logger& aLogger) : CanBuildTower(true), mLogger(aLogger) {}

    /// Called when some contacts occur
    /**
     * @param callbackData Contains information about all the contacts
     */
    virtual void onOverlap(CallbackData&) override;

    bool CanBuildTower;

    std::vector<glm::vec3> Contacts;

   private:
    Logger mLogger;
};

/// Creates a temporary rigid body at aPosition with aColliderParams,
/// tests for overlaps against existing bodies, and returns true if
/// no tower/base collision is detected.
bool CanPlaceTower(
    Physics&              aPhysics,
    const glm::vec3&      aPosition,
    const ColliderParams& aColliderParams,
    const Logger&         aLogger);

#ifndef DOCTEST_CONFIG_DISABLE
#include <doctest.h>

TEST_CASE("tower.can_place")
{
    auto           logger = WATO_NAMED_LOGGER("tower_test");
    static Physics phy(logger);
    phy.Init();
    static constexpr ColliderParams towerCollider{
        .CollisionCategoryBits = Category::Tower,
        .CollideWithMaskBits   = CollidesWith(Category::Tower, Category::Base),
        .IsTrigger             = false,
        .Offset                = Transform3D{},
        .ShapeParams           = BoxShapeParams{.HalfExtents = glm::vec3(0.35f, 0.65f, 0.35f)},
    };
    static constexpr RigidBodyParams bodyParams{
        .Type           = rp3d::BodyType::STATIC,
        .Velocity       = 0.0f,
        .Direction      = glm::vec3(0.0f),
        .GravityEnabled = false,
    };

    static constexpr glm::vec3 pos(1.0f, 0.0f, 1.0f);

    SUBCASE("placement on empty world succeeds")
    {
        CHECK(CanPlaceTower(phy, pos, towerCollider, logger));
    }

    SUBCASE("placement on top of existing tower fails")
    {
        // Place a persistent tower
        auto* body = phy.CreateRigidBody(bodyParams, Transform3D{pos});
        phy.AddCollider(body, towerCollider);

        CHECK_FALSE(CanPlaceTower(phy, pos, towerCollider, logger));
    }

    SUBCASE("placement far from existing tower succeeds")
    {
        auto* body = phy.CreateRigidBody(bodyParams, Transform3D{pos});
        phy.AddCollider(body, towerCollider);

        CHECK(CanPlaceTower(phy, glm::vec3(10.0f, 0.0f, 10.0f), towerCollider, logger));
    }
}

TEST_CASE("tower.can_place_2")
{
    auto           logger = WATO_NAMED_LOGGER("tower_test");
    static Physics phy(logger);
    phy.Init();
    static constexpr ColliderParams towerCollider{
        .CollisionCategoryBits = Category::Tower,
        .CollideWithMaskBits   = CollidesWith(Category::Tower, Category::Base),
        .IsTrigger             = false,
        .Offset                = Transform3D{},
        .ShapeParams           = BoxShapeParams{.HalfExtents = glm::vec3(0.35f, 0.65f, 0.35f)},
    };
    static constexpr RigidBodyParams bodyParams{
        .Type           = rp3d::BodyType::STATIC,
        .Velocity       = 0.0f,
        .Direction      = glm::vec3(0.0f),
        .GravityEnabled = false,
    };

    static constexpr glm::vec3 pos(1.0f, 0.0f, 1.0f);

    SUBCASE("placement on empty world succeeds")
    {
        CHECK(CanPlaceTower(phy, pos, towerCollider, logger));
    }

    SUBCASE("placement on top of existing tower fails")
    {
        // Place a persistent tower
        auto* body = phy.CreateRigidBody(bodyParams, Transform3D{pos});
        phy.AddCollider(body, towerCollider);

        CHECK_FALSE(CanPlaceTower(phy, pos, towerCollider, logger));
    }

    SUBCASE("placement far from existing tower succeeds")
    {
        auto* body = phy.CreateRigidBody(bodyParams, Transform3D{pos});
        phy.AddCollider(body, towerCollider);

        CHECK(CanPlaceTower(phy, glm::vec3(10.0f, 0.0f, 10.0f), towerCollider, logger));
    }
}

#endif
