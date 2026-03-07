#include "core/tower_building_handler.hpp"

#include <spdlog/spdlog.h>

#include "core/physics/physics.hpp"

void TowerBuildingHandler::onOverlap(CallbackData& aCallbackData)
{
    mLogger->trace("checking contacts");
    for (uint32_t pairIdx = 0; pairIdx < aCallbackData.getNbOverlappingPairs(); ++pairIdx) {
        OverlapPair pair = aCallbackData.getOverlappingPair(pairIdx);

        unsigned short c1Category = pair.getCollider1()->getCollisionCategoryBits();
        unsigned short c2Category = pair.getCollider2()->getCollisionCategoryBits();

        mLogger->debug("got overlap pair {}: 0x{:x}-0x{:x}", pairIdx, c1Category, c2Category);

        if (c1Category == c2Category || c1Category == Category::Base
            || c2Category == Category::Base) {
            CanBuildTower = false;
        }
    }
}

bool CanPlaceTower(
    Physics&              aPhysics,
    const glm::vec3&      aPosition,
    const ColliderParams& aColliderParams,
    const Logger&         aLogger)
{
    RigidBodyParams probeParams{
        .Type           = rp3d::BodyType::STATIC,
        .Velocity       = 0.0f,
        .Direction      = glm::vec3(0.0f),
        .GravityEnabled = false,
    };

    auto* body = aPhysics.CreateRigidBody(probeParams, Transform3D{aPosition});
    aPhysics.AddCollider(body, aColliderParams);

    TowerBuildingHandler handler(aLogger);
    aPhysics.World()->testOverlap(body, handler);
    aPhysics.World()->destroyRigidBody(body);

    return handler.CanBuildTower;
}
