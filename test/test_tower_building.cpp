#include <doctest.h>

#include <core/tower_building_handler.hpp>

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

    SUBCASE("empty") { CHECK(CanPlaceTower(phy, pos, towerCollider, logger)); }

    SUBCASE("nok")
    {
        // Place a persistent tower
        auto* body = phy.CreateRigidBody(bodyParams, Transform3D{pos});
        phy.AddCollider(body, towerCollider);

        CHECK_FALSE(CanPlaceTower(phy, pos, towerCollider, logger));
    }

    SUBCASE("ok")
    {
        auto* body = phy.CreateRigidBody(bodyParams, Transform3D{pos});
        phy.AddCollider(body, towerCollider);

        CHECK(CanPlaceTower(phy, glm::vec3(10.0f, 0.0f, 10.0f), towerCollider, logger));
    }
}
