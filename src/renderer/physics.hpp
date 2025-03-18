#pragma once

#include "config.h"
#include "reactphysics3d/utils/DefaultLogger.h"

struct PhysicsParams {
    bool info_logs;
    bool warning_logs;
    bool error_logs;

    reactphysics3d::DefaultLogger* logger;

#if WATO_DEBUG
    bool render_shapes;
    bool render_aabb;
    bool render_contact_points;
    bool render_contact_normals;
#endif
};
