#pragma once

#include "systems/system.hpp"

class NetworkSyncSystem : public System<NetworkSyncSystem>
{
   public:
    void operator()(Registry& aRegistry);

    static constexpr const char* StaticName() { return "NetworkSyncSystem"; }
};
