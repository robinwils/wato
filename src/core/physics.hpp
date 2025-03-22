#pragma once

#include "config.h"
#include "reactphysics3d/reactphysics3d.h"
#include "registry/registry.hpp"

struct PhysicsParams {
    bool InfoLogs;
    bool WarningLogs;
    bool ErrorLogs;

    reactphysics3d::DefaultLogger *Logger;

#if WATO_DEBUG
    bool RenderShapes;
    bool RenderAabb = true;
    bool RenderContactPoints;
    bool RenderContactNormals;
#endif
};

class Physics
{
   public:
    Physics() {}
    Physics(const Physics &)            = delete;
    Physics(Physics &&)                 = delete;
    Physics &operator=(const Physics &) = delete;
    Physics &operator=(Physics &&)      = delete;

    void                               Init(Registry *aRegistry);
    void                               InitLogger();
    [[nodiscard]] rp3d::PhysicsWorld  *World() noexcept { return mWorld; }
    [[nodiscard]] rp3d::PhysicsCommon &Common() noexcept { return mCommon; }

    PhysicsParams Params;

   private:
    rp3d::PhysicsCommon mCommon;
    rp3d::PhysicsWorld *mWorld = nullptr;
};
