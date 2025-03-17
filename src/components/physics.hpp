#pragma once

#include "components/transform3d.hpp"
#include "config.h"
#include "reactphysics3d/reactphysics3d.h"

struct Physics {
    rp3d::PhysicsCommon common;
    rp3d::PhysicsWorld* world = nullptr;

    rp3d::RigidBody* createCollider(const Transform3D& transform, rp3d::BodyType type)
    {
        auto* rb  = world->createRigidBody(transform.to_rp3d());
        auto* box = common.createBoxShape(rp3d::Vector3(0.35f, 0.65f, 0.35f));
        rb->setType(type);
        rb->addCollider(box, rp3d::Transform::identity());

#if WATO_DEBUG
        rb->setIsDebugEnabled(true);
#endif

        return rb;
    }
};
