#include "core/physics/physics.hpp"

#include <fmt/ranges.h>
#include <spdlog/spdlog.h>

void Physics::Init()
{
    mWorld = mCommon.createPhysicsWorld();

    // Create the default logger
    Params.InfoLogs    = false;
    Params.WarningLogs = false;
    Params.ErrorLogs   = true;
    Params.Logger      = mCommon.createDefaultLogger();
#if WATO_DEBUG
    mWorld->setIsDebugRenderingEnabled(true);
#endif
    InitLogger();
}

void Physics::InitLogger()
{
    uint32_t logLevel = 0;
    if (Params.InfoLogs) {
        logLevel |= static_cast<uint32_t>(rp3d::Logger::Level::Information);
    }
    if (Params.WarningLogs) {
        logLevel |= static_cast<uint32_t>(rp3d::Logger::Level::Warning);
    }
    if (Params.ErrorLogs) {
        logLevel |= static_cast<uint32_t>(rp3d::Logger::Level::Error);
    }

    // Output the logs into an HTML file
    Params.Logger->addFileDestination("rp3d_log.html", logLevel, rp3d::DefaultLogger::Format::HTML);

    // Output the logs into the standard output
    Params.Logger->addStreamDestination(std::cout, logLevel, rp3d::DefaultLogger::Format::Text);
    mCommon.setLogger(Params.Logger);
}

rp3d::CollisionShape* Physics::CreateCollisionShape(const ColliderShapeParams& aShapeParams)
{
    rp3d::CollisionShape* shape = nullptr;
    std::visit(
        VariantVisitor{
            [&](const BoxShapeParams& aParams) {
                shape = mCommon.createBoxShape(ToRP3D(aParams.HalfExtents));
            },
            [&](const CapsuleShapeParams& aParams) {
                shape = mCommon.createCapsuleShape(aParams.Radius, aParams.Height);
            },
            [&](const HeightFieldShapeParams& aParams) {
                std::vector<rp3d::Message> messages;
                shape = mCommon.createHeightFieldShape(mCommon.createHeightField(
                    aParams.Columns,
                    aParams.Rows,
                    aParams.Data.data(),
                    rp3d::HeightField::HeightDataType::HEIGHT_FLOAT_TYPE,
                    messages));
            },
        },
        aShapeParams);
    return shape;
}

rp3d::Collider* Physics::AddCollider(rp3d::RigidBody* aBody, const ColliderParams& aParams)
{
    rp3d::CollisionShape* shape    = CreateCollisionShape(aParams.ShapeParams);
    rp3d::Collider*       collider = aBody->addCollider(shape, aParams.Offset.ToRP3D());

    collider->setCollisionCategoryBits(aParams.CollisionCategoryBits);
    collider->setCollideWithMaskBits(aParams.CollideWithMaskBits);
    collider->setIsTrigger(aParams.IsTrigger);

    return collider;
}

rp3d::RigidBody* Physics::CreateRigidBody(
    const RigidBodyParams& aParams,
    const Transform3D&     aTransform)
{
    auto* body = mWorld->createRigidBody(aTransform.ToRP3D());
    body->setType(aParams.Type);
    body->enableGravity(aParams.GravityEnabled);

    if (aParams.Type == rp3d::BodyType::KINEMATIC && aParams.Velocity != 0.0f) {
        body->setLinearVelocity(ToRP3D(aParams.Velocity * aParams.Direction));
    }

    body->setUserData(aParams.Data);

#if WATO_DEBUG
    body->setIsDebugEnabled(true);
#endif

    return body;
}

std::optional<glm::vec3> Physics::RayTerrainIntersection(glm::vec3 aOrigin, glm::vec3 aEnd)
{
    WorldRaycastCallback raycastCb;

    rp3d::Ray ray(ToRP3D(aOrigin), ToRP3D(aEnd));
    mWorld->raycast(ray, &raycastCb, Category::Terrain);
    if (!raycastCb.Hits.empty()) {
        return glm::vec3(raycastCb.Hits[0].x, 0.0f, raycastCb.Hits[0].z);
    } else {
        return std::nullopt;
    }
}

void Physics::ToggleObstacle(const rp3d::Collider* aCollider, Graph& aGraph, bool aAdd)
{
    const rp3d::AABB& box = aCollider->getWorldAABB();
    const GraphCell&  min = GraphCell::FromWorldPoint(box.getMin().x, box.getMin().z);
    const GraphCell&  max = GraphCell::FromWorldPoint(box.getMax().x, box.getMax().z);
    mLogger->trace(
        "toggling obstacle from min {}|{} to max {}|{}",
        box.getMin(),
        min,
        box.getMax(),
        max);

    for (GraphCell::size_type i = min.Location.x; i <= max.Location.x; ++i) {
        for (GraphCell::size_type j = min.Location.y; j <= max.Location.y; ++j) {
            const GraphCell cell(i, j);

            if (aAdd) {
                aGraph.AddObstacle(cell);
            } else {
                aGraph.RemoveObstacle(cell);
            }
        }
    }
}
