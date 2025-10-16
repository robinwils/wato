#include "systems/sync.hpp"

#include "components/game.hpp"
#include "core/net/enet_client.hpp"
#include "core/net/enet_server.hpp"
#include "core/net/net.hpp"
#include "core/snapshot.hpp"
#include "input/action.hpp"
#include "registry/registry.hpp"

template <>
void NetworkSyncSystem<ENetClient>::operator()(Registry& aRegistry)
{
    auto& buf      = aRegistry.ctx().get<GameStateBuffer&>();
    auto& net      = aRegistry.ctx().get<ENetClient&>();
    auto& instance = aRegistry.ctx().get<GameInstance&>();

    if (!net.Running() || !net.Connected()) {
        return;
    }

    const GameState& state         = buf.Latest();
    GameState        filteredState = GameState{.Tick = state.Tick};

    for (const Action& action : state.Actions) {
        if (action.Tag == ActionTag::FixedTime) {
            filteredState.Actions.push_back(action);
        }
    }

    if (!filteredState.Actions.empty() || !filteredState.Snapshot.empty()) {
        net.EnqueueSend(new NetworkEvent<NetworkRequestPayload>{
            .Type     = PacketType::ClientSync,
            .PlayerID = 0,
            .Payload  = SyncPayload{.GameID = instance.GameID, .State = filteredState},
        });
    }
}

template <>
void NetworkSyncSystem<ENetServer>::operator()(Registry& aRegistry)
{
    auto& buf          = aRegistry.ctx().get<GameStateBuffer&>();
    auto& net          = aRegistry.ctx().get<ENetServer&>();
    auto& instance     = aRegistry.ctx().get<GameInstance&>();
    auto& rbStorage    = aRegistry.storage<entt::reactive>("rigid_bodies_observer"_hs);
    auto  snapshotView = entt::basic_view{rbStorage};

    GameState         serverState = GameState{.Tick = buf.Latest().Tick};
    ByteOutputArchive outAr(serverState.Snapshot);

    entt::snapshot{aRegistry}
        .get<Transform3D>(outAr, rbStorage.begin(), rbStorage.end())
        .get<RigidBody>(outAr, rbStorage.begin(), rbStorage.end())
        .get<Collider>(outAr, rbStorage.begin(), rbStorage.end());

    net.EnqueueResponse(new NetworkEvent<NetworkResponsePayload>{
        .Type     = PacketType::ServerSync,
        .PlayerID = 0,
        .Payload  = SyncPayload{.GameID = instance.GameID, .State = serverState},
    });
}
