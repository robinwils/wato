#include "systems/sync.hpp"

#include "components/game.hpp"
#include "core/net/enet_client.hpp"
#include "core/net/net.hpp"
#include "core/snapshot.hpp"
#include "input/action.hpp"
#include "registry/registry.hpp"

void NetworkSyncSystem::operator()(Registry& aRegistry)
{
    auto& buf       = aRegistry.ctx().get<GameStateBuffer&>();
    auto& netClient = aRegistry.ctx().get<ENetClient&>();
    auto& instance  = aRegistry.ctx().get<GameInstance&>();

    if (!netClient.Running() || !netClient.Connected()) {
        return;
    }

    const GameState&     state = buf.Latest();
    std::vector<uint8_t> storage;
    GameState            filteredState = state;

    filteredState.Actions.clear();
    for (const Action& action : state.Actions) {
        if (action.Tag == ActionTag::FixedTime) {
            filteredState.Actions.emplace_back(action);
        }
    }

    if (!filteredState.Actions.empty()) {
        netClient.EnqueueSend(new NetworkEvent<NetworkRequestPayload>{
            .Type     = PacketType::ClientSync,
            .PlayerID = 0,
            .Payload  = ClientSyncRequest{.GameID = instance.GameID, .State = filteredState},
        });
    }
}
