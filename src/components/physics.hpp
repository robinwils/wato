#pragma once

#include "config.h"
#include "reactphysics3d/reactphysics3d.h"

struct Physics {
    rp3d::PhysicsCommon Common;
    rp3d::PhysicsWorld* World = nullptr;

    bool InfoLogs;
    bool WarningLogs;
    bool ErrorLogs;

    reactphysics3d::DefaultLogger* Logger;

#if WATO_DEBUG
    bool RenderShapes;
    bool RenderAabb = true;
    bool RenderContactPoints;
    bool RenderContactNormals;
#endif
};
