#pragma once

#include <reactphysics3d/reactphysics3d.h>

#include <glm/glm.hpp>
#include <variant>
#include <vector>

struct BoxShapeParams {
    glm::vec3 HalfExtents;
};

struct CapsuleShapeParams {
    float Radius;
    float Height;
};

struct HeightFieldShapeParams {
    std::vector<float> Data;
    int                Rows;
    int                Columns;
};

using ColliderShapeParams =
    std::variant<BoxShapeParams, CapsuleShapeParams, HeightFieldShapeParams>;

struct PhysicsParams {
    bool InfoLogs    = false;
    bool WarningLogs = false;
    bool ErrorLogs   = true;

    rp3d::DefaultLogger* Logger = nullptr;

#if WATO_DEBUG
    bool RenderShapes         = false;
    bool RenderAabb           = true;
    bool RenderContactPoints  = false;
    bool RenderContactNormals = false;
#endif
};
