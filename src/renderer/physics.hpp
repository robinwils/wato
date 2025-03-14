#pragma once

#include "config.h"

#if WATO_DEBUG
struct DebugRendererParams {
    bool render_shapes;
    bool render_aabb;
};
#endif
