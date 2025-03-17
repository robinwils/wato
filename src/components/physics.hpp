#pragma once

#include "reactphysics3d/reactphysics3d.h"

struct Physics {
    rp3d::PhysicsCommon common;
    rp3d::PhysicsWorld* world = nullptr;
};
