#pragma once

#include "reactphysics3d/reactphysics3d.h"

struct Physics {
    rp3d::PhysicsCommon Common;
    rp3d::PhysicsWorld* World = nullptr;
};
