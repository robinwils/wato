#pragma once

#include <bx/spscqueue.h>

#include <atomic>

#include "core/net/net.hpp"
#include "core/options.hpp"
#include "core/physics.hpp"
#include "registry/registry.hpp"
#include "systems/creep.hpp"
#include "systems/physics.hpp"
#include "systems/system.hpp"

class Application
{
   public:
    explicit Application(int aWidth, int aHeight, char** aArgv) : mRunning(false)
    {
        mRegistry.ctx().emplace<Options>(aArgv);
        mRegistry.ctx().emplace<Physics>();
    }
    virtual ~Application() = default;

    Application(const Application&)            = delete;
    Application(Application&&)                 = delete;
    Application& operator=(const Application&) = delete;
    Application& operator=(Application&&)      = delete;

    virtual void Init() = 0;
    virtual int  Run()  = 0;

   protected:
    Registry mRegistry;

    PhysicsSystem         mPhysicsSystem;
    UpdateTransformsSytem mUpdateTransformsSystem;
    CreepSystem           mCreepSystem;

    SystemRegistry mSystems;
    SystemRegistry mSystemsFT;

    std::atomic_bool mRunning;
};
