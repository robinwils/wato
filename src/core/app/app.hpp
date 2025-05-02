#pragma once

#include <bx/spscqueue.h>

#include <atomic>
#include <chrono>

#include "core/options.hpp"
#include "systems/action.hpp"
#include "systems/creep.hpp"
#include "systems/physics.hpp"
#include "systems/system.hpp"

class Application
{
   public:
    static constexpr float kTimeStep = 1.0f / 60.0f;

    explicit Application(char** aArgv) : mOptions(aArgv), mRunning(false) {}
    virtual ~Application() = default;

    Application(const Application&)            = delete;
    Application(Application&&)                 = delete;
    Application& operator=(const Application&) = delete;
    Application& operator=(Application&&)      = delete;

    virtual void Init() = 0;
    virtual int  Run()  = 0;

   protected:
    using clock_type = std::chrono::steady_clock;
    Options mOptions;

    PhysicsSystem             mPhysicsSystem;
    UpdateTransformsSytem     mUpdateTransformsSystem;
    CreepSystem               mCreepSystem;
    DeterministicActionSystem mFTActionSystem;

    SystemRegistry mSystems;
    SystemRegistry mSystemsFT;

    std::atomic_bool mRunning;
};
