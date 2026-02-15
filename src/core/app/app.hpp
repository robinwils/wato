#pragma once

#include <bx/spscqueue.h>

#include <atomic>
#include <chrono>
#include <entt/core/fwd.hpp>
#include <glm/vec2.hpp>
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

    void StartGameInstance(Registry& aRegistry, const GameInstanceID aGameID);
    void StopGameInstance(Registry& aRegistry);
    void SpawnTerrain(
        Registry&           aRegistry,
        const entt::entity& aPlayer,
        const glm::uvec2&   aSize,
        const glm::vec2&    aOffset);

    void SetupObservers(Registry& aRegistry);

    Options mOptions;

    // Time-based (variable frame rate)
    FrameSystemExecutor mFrameExecutor;
    FrameSystemExecutor mMenuExecutor;

    std::atomic_bool mRunning;

    Logger mLogger;
};
