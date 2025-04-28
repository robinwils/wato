#include "systems/sync.hpp"

#include "core/net/enet_client.hpp"
#include "core/snapshot.hpp"
#include "input/action.hpp"
#include "registry/registry.hpp"

void NetworkSyncSystem::operator()(Registry& aRegistry, const float aDeltaTime)
{
    auto&                actions       = aRegistry.ctx().get<ActionBuffer&>();
    auto&                netClient     = aRegistry.ctx().get<ENetClient&>();
    const PlayerActions& latestActions = actions.Latest();
    std::vector<uint8_t> storage;
    ByteOutputArchive    outAr(storage);

    for (const auto& action : latestActions.Actions) {
        outAr(action);
    }

    outAr.Bytes();
}
