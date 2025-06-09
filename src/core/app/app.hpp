#pragma once

#include <bx/spscqueue.h>

#include <atomic>
#include <chrono>

#include "core/options.hpp"
#include "core/types.hpp"
#include "registry/registry.hpp"
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

    virtual void Init();
    virtual int  Run() = 0;

   protected:
    using clock_type = std::chrono::steady_clock;

    void StartGameInstance(Registry& aRegistry, const GameInstanceID aGameID, const bool aIsServer);
    void AdvanceSimulation(Registry& aRegistry, const float aDeltaTime);
    void SpawnMap(Registry& aRegistry, uint32_t aWidth, uint32_t aHeight);

    Options mOptions;

    PhysicsSystem         mPhysicsSystem;
    UpdateTransformsSytem mUpdateTransformsSystem;
    CreepSystem           mCreepSystem;

    SystemRegistry mSystems;
    SystemRegistry mSystemsFT;

    std::atomic_bool mRunning;
};
