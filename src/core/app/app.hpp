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
#include "systems/system_executor.hpp"

class Application
{
   public:
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
    void StopGameInstance(Registry& aRegistry);

    void SpawnMap(Registry& aRegistry, uint32_t aWidth, uint32_t aHeight);

    void SetupObservers(Registry& aRegistry);

    virtual void OnGameInstanceCreated(Registry& aRegistry) = 0;

    Options mOptions;

    FrameSystemExecutor mFrameExecutor;  // Time-based (variable frame rate)

    std::atomic_bool mRunning;

    Logger mLogger;
};
