#include "core/net/enet_client.hpp"

#include <enet.h>
#include <fmt/format.h>
#include <spdlog/spdlog.h>

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

    enet_address_set_host(&address, "127.0.0.1");
    address.port = 7777;

    /* Initiate the connection, allocating the two channels 0 and 1. */
    mPeer = enet_host_connect(mHost.get(), &address, 2, 0);
    return mPeer != nullptr;
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
        mLogger->info("forcing disconnect");
        enet_peer_reset(mPeer);
    }
    mConnected = false;
    mRunning   = false;
}

void ENetClient::OnConnect(ENetEvent& aEvent)
{
    BX_UNUSED(aEvent);
    // TODO: better player ID handling
    mRespChannel.Send(new NetworkResponse{
        .Type     = PacketType::Connected,
        .PlayerID = 0,
        .Payload  = ConnectedResponse{},
    });
    mConnected = true;
}

void ENetClient::OnReceive(ENetEvent& aEvent)
{
    ByteInputArchive archive(std::span<uint8_t>(aEvent.packet->data, aEvent.packet->dataLength));
    auto*            ev = new NetworkResponse;

    NetworkResponse::Deserialize(archive, ev);

    EnqueueResponse(ev);
}

void ENetClient::OnDisconnect(ENetEvent& aEvent)
{
    BX_UNUSED(aEvent);
    mConnected = false;
    mRunning   = false;
}

void ENetClient::OnDisconnectTimeout(ENetEvent& aEvent) { BX_UNUSED(aEvent); }

void ENetClient::OnNone(ENetEvent& aEvent) { BX_UNUSED(aEvent); }
