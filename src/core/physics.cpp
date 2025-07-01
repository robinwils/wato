#include "core/physics.hpp"

#include <spdlog/spdlog.h>

#include "components/rigid_body.hpp"

void Physics::Init(Registry& aRegistry)
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
    aRegistry.on_destroy<RigidBody>().connect<&Physics::DeleteRigidBody>(this);
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

rp3d::RigidBody* Physics::CreateRigidBody(
    const entt::entity&   aEntity,
    Registry&             aRegistry,
    const RigidBodyParams aParams)
{
    auto* body = mWorld->createRigidBody(aParams.Transform);
    body->setType(aParams.Type);
    body->enableGravity(aParams.GravityEnabled);

    // destroyed in on_destroy listener
    body->setUserData(new RigidBodyData(aEntity));
#if WATO_DEBUG
    body->setIsDebugEnabled(true);
#endif

    aRegistry.emplace<RigidBody>(aEntity, body);
    return body;
}

rp3d::Collider*
Physics::AddBoxCollider(rp3d::RigidBody* aBody, const rp3d::Vector3& aSize, const bool aIsTrigger)
{
    auto* box      = mCommon.createBoxShape(aSize);
    auto* collider = aBody->addCollider(box, rp3d::Transform::identity());

    collider->setIsTrigger(aIsTrigger);
    return collider;
}

rp3d::Collider* Physics::AddCapsuleCollider(
    rp3d::RigidBody* aBody,
    const float&     aRadius,
    const float&     aHeight,
    const bool       aIsTrigger)
{
    auto* box      = mCommon.createCapsuleShape(aRadius, aHeight);
    auto* collider = aBody->addCollider(box, rp3d::Transform::identity());

    collider->setIsTrigger(aIsTrigger);
    return collider;
}

void Physics::DeleteRigidBody(Registry& aRegistry, entt::entity aEntity)
{
    if (const auto& body = aRegistry.get<RigidBody>(aEntity);
        body.RigidBody && body.RigidBody->getUserData()) {
        auto* uData = static_cast<RigidBodyData*>(body.RigidBody->getUserData());
        delete uData;
        aRegistry.ctx().get<Physics&>().World()->destroyRigidBody(body.RigidBody);
    }
}
