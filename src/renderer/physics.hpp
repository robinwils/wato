#pragma once

#include "config.h"
#include "reactphysics3d/utils/DefaultLogger.h"

struct PhysicsParams {
    bool InfoLogs;
    bool WarningLogs;
    bool ErrorLogs;

    reactphysics3d::DefaultLogger* Logger;

#if WATO_DEBUG
    bool RenderShapes;
    bool RenderAabb;
    bool RenderContactPoints;
    bool RenderContactNormals;
#endif
};
