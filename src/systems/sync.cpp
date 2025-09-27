#include "systems/sync.hpp"

#include "core/net/enet_client.hpp"
#include "core/net/net.hpp"
#include "core/snapshot.hpp"
#include "input/action.hpp"
#include "registry/registry.hpp"

void NetworkSyncSystem::operator()(Registry& aRegistry, const float aDeltaTime)
{
    auto& actions   = aRegistry.ctx().get<ActionBuffer&>();
    auto& netClient = aRegistry.ctx().get<ENetClient&>();

    if (!netClient.Running() || !netClient.Connected()) {
        return;
    }

    const PlayerActions& latestActions = actions.Latest();
    std::vector<uint8_t> storage;
    PlayerActions        filteredActions = latestActions;

    filteredActions.Actions.clear();
    for (const Action& action : latestActions.Actions) {
        if (action.Tag == ActionTag::FixedTime) {
            filteredActions.Actions.emplace_back(action);
        }
    }

    if (!filteredActions.Actions.empty()) {
        netClient.EnqueueSend(new NetworkEvent<NetworkRequestPayload>{
            .Type     = PacketType::Actions,
            .PlayerID = 0,
            .Payload  = filteredActions,
        });
    }
}
