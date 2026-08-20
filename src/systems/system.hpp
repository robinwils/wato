#pragma once

#include <chrono>
#include <cstdint>
#include <entt/process/process.hpp>
#include <type_traits>
#include <typeinfo>

#include "registry/registry.hpp"

// Tick rate — matches Application::kTimeStep (60 FPS)
using GameTick = std::chrono::duration<std::uint32_t, std::ratio<1, 60>>;

/**
 * @brief Base class for ECS systems
 *
 * Delta = std::uint32_t → fixed timestep (absolute tick number)
 * Delta = float         → frame time (seconds)
 */
template <typename Delta>
class System : public entt::basic_process<Delta>
{
   public:
    template <typename Allocator>
    explicit System(const Allocator& /*unused*/)
    {
    }

    System() = default;

    void update(Delta aDelta, void* aData) override
    {
        auto* registry = static_cast<Registry*>(aData);
        if (!mLogger) {
            initLogger(*registry);
        }
        Execute(*registry, aDelta);
    }

    [[nodiscard]] virtual const char* Name() const { return typeid(*this).name(); }

   protected:
    virtual void Execute(Registry& aRegistry, Delta aDelta) = 0;

    Logger mLogger;

   private:
    void initLogger(const Registry& aRegistry)
    {
        const auto& sideLogger = WATO_REG_LOGGER(aRegistry);

        auto name = fmt::format("{}.{}", sideLogger->name(), Name());

        if (auto existing = spdlog::get(name)) {
            mLogger = existing;
            return;
        }

        mLogger = std::make_shared<spdlog::logger>(
            name,
            sideLogger->sinks().begin(),
            sideLogger->sinks().end());
        mLogger->set_level(sideLogger->level());
        spdlog::register_logger(mLogger);
    }
};

/**
 * @brief System that fires at a fixed interval
 *
 * For integral Delta (ticks): uses modulo on absolute tick number.
 * For floating Delta (seconds): uses accumulator on relative delta.
 *
 * Interval is specified via std::chrono::duration and converted at construction.
 * FireImmediately controls whether the first execution happens at tick 0 / time 0
 * or waits for the first full interval.
 */
template <typename Delta>
class PeriodicSystem : public System<Delta>
{
   public:
    template <typename Allocator, typename Rep, typename Period>
    PeriodicSystem(
        const Allocator&                   aAlloc,
        std::chrono::duration<Rep, Period> aInterval,
        bool                               aFireImmediately = false)
        : System<Delta>(aAlloc), mInterval(toDelta(aInterval)), mFireImmediately(aFireImmediately)
    {
    }

    template <typename Rep, typename Period>
    explicit PeriodicSystem(
        std::chrono::duration<Rep, Period> aInterval,
        bool                               aFireImmediately = false)
        : mInterval(toDelta(aInterval)), mFireImmediately(aFireImmediately)
    {
    }

   protected:
    void Execute(Registry& aRegistry, Delta aDelta) final
    {
        if constexpr (std::is_integral_v<Delta>) {
            if (mInterval > 0 && aDelta % mInterval == 0 && (aDelta > 0 || mFireImmediately)) {
                PeriodicExecute(aRegistry, aDelta);
            }
        } else {
            if (mFirst && mFireImmediately) {
                mFirst = false;
                PeriodicExecute(aRegistry, aDelta);
            }
            mAccumulated += aDelta;
            if (mAccumulated >= mInterval) {
                mAccumulated -= mInterval;
                PeriodicExecute(aRegistry, aDelta);
            }
        }
    }

    virtual void PeriodicExecute(Registry& aRegistry, Delta aDelta) = 0;

   private:
    template <typename Rep, typename Period>
    static Delta toDelta(std::chrono::duration<Rep, Period> aDelta)
    {
        if constexpr (std::is_integral_v<Delta>) {
            return std::chrono::duration_cast<GameTick>(aDelta).count();
        } else {
            return std::chrono::duration<float>(aDelta).count();
        }
    }

    Delta mInterval;
    bool  mFireImmediately;
    bool  mFirst{true};
    Delta mAccumulated{};
};

using FixedSystem         = System<std::uint32_t>;
using FrameSystem         = System<float>;
using PeriodicFixedSystem = PeriodicSystem<std::uint32_t>;
using PeriodicFrameSystem = PeriodicSystem<float>;
