#pragma once

#include <entt/process/scheduler.hpp>

#include "registry/registry.hpp"

/**
 * @brief Execution backend for systems
 *
 * Currently uses entt::basic_scheduler.
 * TODO:
 * - Organizer + Taskflow for parallel execution
 *
 * EnTT 3.16+ uses basic_process<Delta> without CRTP!
 */
template <typename Delta>
class SystemExecutor
{
   public:
    SystemExecutor() = default;

    /**
     * @brief Register a system for execution
     *
     * Systems execute in registration order. Use the returned reference
     * to chain dependencies with then<>() if needed.
     *
     * @tparam System Type of system (must inherit from entt::basic_process<Delta>)
     * @tparam Args Constructor argument types
     * @param args Arguments to forward to system constructor
     * @return Reference to scheduler for chaining with then<>()
     *
     * @example
     * // Simple registration (execution order = registration order)
     * executor.Register<PhysicsSystem>();
     * executor.Register<UpdateTransformsSystem>();  // Runs after Physics
     *
     * // Explicit chaining (for conditional sequences)
     * executor.Register<PhysicsSystem>()
     *     .then<UpdateTransformsSystem>();  // Only if Physics succeeds
     */
    template <typename System, typename... Args>
    auto& Register(Args&&... aRgs)
    {
        return mScheduler.template attach<System>(std::forward<Args>(aRgs)...);
    }

    /**
     * @brief Update all registered systems
     * @param delta Time delta (tick number for uint32_t, seconds for float)
     * @param data Opaque pointer (typically Registry*)
     */
    void Update(Delta aDelta, void* aData) { mScheduler.update(aDelta, aData); }

   private:
    // Current backend: scheduler
    // Future: add organizer + storage + taskflow
    entt::basic_scheduler<Delta> mScheduler;
};

/**
 * @brief Executor for fixed timestep systems (tick-based)
 *
 * Systems receive absolute tick number (0, 1, 2, 3...).
 * Use for deterministic gameplay logic.
 */
using FixedSystemExecutor = SystemExecutor<std::uint32_t>;

/**
 * @brief Executor for frame time systems (time-based)
 *
 * Systems receive delta time in seconds.
 * Use for rendering, animation, UI, etc.
 */
using FrameSystemExecutor = SystemExecutor<float>;
