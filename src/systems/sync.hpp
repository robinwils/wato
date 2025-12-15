#pragma once

#include "systems/system.hpp"

template <typename _ENetT>
class NetworkSyncSystem : public System<NetworkSyncSystem<_ENetT>>
{
   public:
    void operator()(Registry& aRegistry);

    static constexpr const char* StaticName() { return "NetworkSyncSystem"; }
};
