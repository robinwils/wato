#include "core/net/enet_server.hpp"

#include <bx/spscqueue.h>
#include <enet.h>
#include <spdlog/spdlog.h>

#include <span>
#include <stdexcept>

#include "components/player.hpp"
#include "core/net/net.hpp"
#include "core/snapshot.hpp"
#include "core/sys/log.hpp"

void ENetServer::Init()
{
    ENetBase::Init();

    // Bind the server to the default localhost.
    ENetAddress address = {.host = ENET_HOST_ANY, .port = 7777, .sin6_scope_id = 0};

    mHost = enet_host_ptr{enet_host_create(&address, 128, 2, 0, 0)};

    if (!mHost) {
        throw std::runtime_error("An error occurred while trying to create an ENet server host.");
    }
}

void ENetServer::ConsumeNetworkResponses()
{
    while (NetworkEvent<NetworkResponsePayload>* ev = mRespQueue.pop()) {
        // write header
        ByteOutputArchive archive;
        archive.Write<PacketType>(&ev->Type, sizeof(ev->Type));
        archive.Write<PlayerID>(&ev->PlayerID, sizeof(ev->PlayerID));

        // write payload
        std::visit(
            VariantVisitor{
                [&](const ConnectedResponse& aResp) {},
                [&](const NewGameResponse& aResp) { NewGameResponse::Serialize(archive, aResp); },
            },
            ev->Payload);

        if (!mConnectedPeers.contains(ev->PlayerID)) {
            throw std::runtime_error(fmt::format("player {} is not connected", ev->PlayerID));
        } else {
            Send(mConnectedPeers[ev->PlayerID], archive.Bytes());
        }
        delete ev;
    }
}

void ENetServer::OnConnect(ENetEvent& aEvent) {}

void ENetServer::OnReceive(ENetEvent& aEvent)
{
    ByteInputArchive archive(std::span<uint8_t>(aEvent.packet->data, aEvent.packet->dataLength));
    auto*            ev = new NetworkEvent<NetworkRequestPayload>();

    archive.Read<PacketType>(&ev->Type, sizeof(PacketType));
    archive.Read<PlayerID>(&ev->PlayerID, sizeof(PlayerID));
    switch (ev->Type) {
        case PacketType::Actions: {
            PlayerActions actions;
            PlayerActions::Deserialize(archive, actions);
            ev->Payload = actions;
            break;
        }
        case PacketType::NewGame:
            NewGameRequest payload;
            NewGameRequest::Deserialize(archive, payload);
            ev->Payload = payload;
            if (!mConnectedPeers.contains(payload.PlayerAID)) {
                mConnectedPeers[payload.PlayerAID] = aEvent.peer;
            }
            break;
        default:
            break;
    }
    mQueue.push(ev);
}

void ENetServer::OnDisconnect(ENetEvent& aEvent) {}

void ENetServer::OnDisconnectTimeout(ENetEvent& aEvent) {}

void ENetServer::OnNone(ENetEvent& aEvent) {}
