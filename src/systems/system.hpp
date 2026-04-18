#pragma once

#include <chrono>
#include <cstdint>
#include <entt/process/process.hpp>
#include <memory>
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
    explicit System(const Allocator&)
    {
    }

    System() = default;

    void update(Delta delta, void* data) override
    {
        auto* registry = static_cast<Registry*>(data);
        Execute(*registry, delta);
    }

    virtual const char* Name() const { return typeid(*this).name(); }

   protected:
    virtual void Execute(Registry& registry, Delta delta) = 0;
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
        const Allocator&                   a,
        std::chrono::duration<Rep, Period> interval,
        bool                               fireImmediately = false)
        : System<Delta>(a), mInterval(ToDelta(interval)), mFireImmediately(fireImmediately)
    {
    }

    template <typename Rep, typename Period>
    explicit PeriodicSystem(
        std::chrono::duration<Rep, Period> interval,
        bool                               fireImmediately = false)
        : mInterval(ToDelta(interval)), mFireImmediately(fireImmediately)
    {
    }

   protected:
    void Execute(Registry& registry, Delta delta) final
    {
        if constexpr (std::is_integral_v<Delta>) {
            if (mInterval > 0 && delta % mInterval == 0 && (delta > 0 || mFireImmediately)) {
                PeriodicExecute(registry, delta);
            }
        } else {
            if (mFirst && mFireImmediately) {
                mFirst = false;
                PeriodicExecute(registry, delta);
            }
            mAccumulated += delta;
            if (mAccumulated >= mInterval) {
                mAccumulated -= mInterval;
                PeriodicExecute(registry, delta);
            }
        }
    }

    virtual void PeriodicExecute(Registry& registry, Delta delta) = 0;

   private:
    template <typename Rep, typename Period>
    static Delta ToDelta(std::chrono::duration<Rep, Period> d)
    {
        if constexpr (std::is_integral_v<Delta>) {
            return std::chrono::duration_cast<GameTick>(d).count();
        } else {
            return std::chrono::duration<float>(d).count();
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
