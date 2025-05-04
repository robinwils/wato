#include "core/net/enet_client.hpp"

#include <bx/string.h>
#include <enet.h>
#include <fmt/base.h>

#include <stdexcept>
#include <variant>

#include "core/net/net.hpp"
#include "core/snapshot.hpp"
#include "core/sys/log.hpp"

void ENetClient::Init()
{
    ENetBase::Init();

    mHost = enet_host_ptr{enet_host_create(
        nullptr /* create a client host */,
        1 /* only allow 1 outgoing connection */,
        2 /* allow up 2 channels to be used, 0 and 1 */,
        0 /* assume any amount of incoming bandwidth */,
        0 /* assume any amount of outgoing bandwidth */)};
    if (!mHost) {
        throw std::runtime_error("An error occurred while trying to create an ENet client host.");
    }
}

bool ENetClient::Connect()
{
    ENetAddress address{};

    /* Connect to some.server.net:1234. */
    enet_address_set_host(&address, "127.0.0.1");
    address.port = 7777;

    /* Initiate the connection, allocating the two channels 0 and 1. */
    mPeer = enet_host_connect(mHost.get(), &address, 2, 0);
    return mPeer != nullptr;
}

void ENetClient::send(const std::vector<uint8_t> aData)
{
    if (mPeer == nullptr) {
        DBG("client peer not initialized");
        return;
    }

    ENetPacket* packet = enet_packet_create(aData.data(), aData.size(), ENET_PACKET_FLAG_RELIABLE);

    if (-1 == enet_peer_send(mPeer, 0, packet)) {
        enet_packet_destroy(packet);
    }

    enet_host_flush(mHost.get());
}

void ENetClient::Disconnect()
{
    if (mPeer != nullptr) {
        enet_peer_disconnect(mPeer, 0);
    }
}

void ENetClient::ForceDisconnect()
{
    if (mPeer != nullptr) {
        INFO("forcing disconnect");
        enet_peer_reset(mPeer);
    }
    mConnected = false;
    mRunning   = false;
}

void ENetClient::ConsumeNetworkEvents()
{
    while (NetworkEvent<NetworkRequestPayload>* ev = mQueue.pop()) {
        // write header
        ByteOutputArchive archive;
        archive.Write<int>(&ev->Type, sizeof(ev->Type));

        // write payload
        std::visit(
            EventVisitor{
                [&](const PlayerActions& aActions) { PlayerActions::Serialize(archive, aActions); },
                [&](const NewGameRequest& aNGPayloa) {},
            },
            ev->Payload);
        send(archive.Bytes());
        delete ev;
    }
}

void ENetClient::OnConnect(ENetEvent& aEvent) { mConnected = true; }

void ENetClient::OnReceive(ENetEvent& aEvent)
{
    ByteInputArchive archive(std::span<uint8_t>(aEvent.packet->data, aEvent.packet->dataLength));
    auto*            ev = new NetworkEvent<NetworkResponsePayload>();

    archive.Read<PacketType>(&ev->Type, sizeof(PacketType));
    switch (ev->Type) {
        case PacketType::NewGame:
            NewGameResponse resp;
            NewGameResponse::Deserialize(archive, resp);
            break;
        default:
            fmt::println("unknown packet type");
            delete ev;
            return;
    }
    mRespQueue.push(ev);
}

void ENetClient::OnDisconnect(ENetEvent& aEvent)
{
    mConnected = false;
    mRunning   = false;
}

void ENetClient::OnDisconnectTimeout(ENetEvent& aEvent) {}

void ENetClient::OnNone(ENetEvent& aEvent) {}
