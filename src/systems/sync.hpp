#pragma once

#include "systems/system.hpp"

/**
 * @brief Network synchronization system (fixed timestep)
 *
 * Syncs game state between client and server.
 * Runs at deterministic 60 FPS.
 *
 * Template parameter _ENetT is either ENetClient or ENetServer.
 */
template <typename _ENetT>
class NetworkSyncSystem : public FixedSystem
{
   public:
    using FixedSystem::FixedSystem;

   protected:
    void Execute(Registry& aRegistry, std::uint32_t aTick) override;
};
