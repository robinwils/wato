#pragma once

#include <cstdint>
#include <entt/process/process.hpp>
#include <memory>
#include <typeinfo>

#include "registry/registry.hpp"

/**
 * @brief Base class for fixed timestep systems (tick-based)
 *
 * Fixed timestep systems run at a constant rate (60 FPS).
 * They receive the absolute tick number (0, 1, 2, 3...).
 *
 * Use for deterministic gameplay logic:
 * - Physics
 * - AI
 * - Networking
 * - Game logic
 *
 */
class FixedSystem : public entt::basic_process<std::uint32_t>
{
   public:
    // Constructor accepting allocator (required by entt::scheduler)
    template <typename Allocator>
    explicit FixedSystem(const Allocator&)
    {
    }

    // Default constructor
    FixedSystem() = default;

    /**
     * @brief Called by entt::basic_process every fixed tick
     * @param tick Absolute tick number (0, 1, 2, 3...)
     * @param data Opaque pointer (Registry*)
     */
    void update(std::uint32_t tick, void* data) override
    {
        auto* registry = static_cast<Registry*>(data);
        Execute(*registry, tick);
    }

    /**
     * @brief Get the name of this system
     *
     * Default implementation uses typeid (mangled name).
     * Override for cleaner debug output.
     */
    virtual const char* Name() const { return typeid(*this).name(); }

   protected:
    /**
     * @brief Main execution function for fixed timestep
     * @param registry ECS registry
     * @param tick Absolute tick number (deterministic)
     */
    virtual void Execute(Registry& registry, std::uint32_t tick) = 0;
};

/**
 * @brief Base class for frame time systems (time-based)
 *
 * Frame time systems run at variable rate based on actual frame time.
 * They receive the delta time in seconds (or interpolation factor).
 *
 * Use for non-deterministic, cosmetic systems:
 * - Rendering
 * - Input
 * - UI
 * - Animation
 * - Audio/VFX
 */
class FrameSystem : public entt::basic_process<float>
{
   public:
    // Constructor accepting allocator (required by entt::scheduler)
    template <typename Allocator>
    explicit FrameSystem(const Allocator&)
    {
    }

    // Default constructor
    FrameSystem() = default;

    /**
     * @brief Called by entt::basic_process every frame
     * @param delta Time delta in seconds (or interpolation factor)
     * @param data Opaque pointer (Registry*)
     */
    void update(float delta, void* data) override
    {
        auto* registry = static_cast<Registry*>(data);
        Execute(*registry, delta);
    }

    /**
     * @brief Get the name of this system
     *
     * Default implementation uses typeid (mangled name).
     * Override for cleaner debug output.
     */
    virtual const char* Name() const { return typeid(*this).name(); }

   protected:
    /**
     * @brief Main execution function for frame time
     * @param registry ECS registry
     * @param delta Time delta in seconds
     */
    virtual void Execute(Registry& registry, float delta) = 0;
};
