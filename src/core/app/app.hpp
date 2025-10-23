#pragma once

#include <bx/spscqueue.h>

#include <atomic>
#include <chrono>
#include <entt/core/fwd.hpp>
#include <taskflow/core/executor.hpp>

#include "core/options.hpp"
#include "core/sys/log.hpp"
#include "core/types.hpp"
#include "registry/registry.hpp"
#include "systems/ai.hpp"
#include "systems/creep.hpp"
#include "systems/physics.hpp"
#include "systems/system.hpp"

class Application
{
   public:
    static constexpr float kTimeStep = 1.0f / 60.0f;

    explicit Application(const std::string& aName)
        : mRunning(false), mLogger(CreateLogger(aName, "info"))
    {
    }
    explicit Application(const std::string& aName, char** aArgv)
        : mOptions(aArgv), mRunning(false), mLogger(CreateLogger(aName, mOptions.LogLevel()))
    {
    }
    explicit Application(const std::string& aName, const Options& aOptions)
        : mOptions(std::move(aOptions)),
          mRunning(false),
          mLogger(CreateLogger(aName, mOptions.LogLevel()))
    {
    }
    virtual ~Application() = default;

    Application(const Application&)            = delete;
    Application(Application&&)                 = delete;
    Application& operator=(const Application&) = delete;
    Application& operator=(Application&&)      = delete;

    virtual void Init();
    virtual int  Run(tf::Executor& aExecutor) = 0;

   protected:
    using clock_type = std::chrono::steady_clock;

    void StartGameInstance(Registry& aRegistry, const GameInstanceID aGameID, const bool aIsServer);
    void AdvanceSimulation(Registry& aRegistry, const float aDeltaTime);
    void StopGameInstance(Registry& aRegistry);

    void SpawnMap(Registry& aRegistry, uint32_t aWidth, uint32_t aHeight);

    void SetupObservers(Registry& aRegistry);
    void ClearAllObservers(Registry& aRegistry);

    virtual void OnGameInstanceCreated() = 0;

    Options mOptions;

    PhysicsSystem         mPhysicsSystem;
    UpdateTransformsSytem mUpdateTransformsSystem;

    SystemRegistry mSystems;
    SystemRegistry mSystemsFT;

    std::atomic_bool mRunning;

    std::vector<entt::hashed_string> mObserverNames;

    Logger mLogger;
};
