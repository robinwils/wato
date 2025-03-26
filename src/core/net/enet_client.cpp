#include "core/net/enet_client.hpp"

#include <bx/string.h>
#include <enet.h>

#include <stdexcept>

#include "core/sys/log.hpp"

using namespace std::literals::chrono_literals;

void ENetClient::Init()
{
    if (enet_initialize() != 0) {
        throw std::runtime_error("failed to initialize Enet");
    }

    mHost = enet_host_ptr{enet_host_create(nullptr /* create a client host */,
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

void ENetClient::Send()
{
    if (mPeer == nullptr) {
        DBG("client peer not initialized");
        return;
    }
    /* Create a reliable packet of size 7 containing "packet\0" */
    ENetPacket* packet =
        enet_packet_create("packet", strlen("packet") + 1, ENET_PACKET_FLAG_RELIABLE);

    /* Extend the packet so and append the string "foo", so it now */
    /* contains "packetfoo\0"                                      */
    // enet_packet_resize(packet, strlen("packetfoo") + 1);
    // bx::strCopy(&packet->data[strlen("packet")], "foo");

    /* Send the packet to the peer over channel id 0. */
    /* One could also broadcast the packet by         */
    /* enet_host_broadcast (host, 0, packet);         */
    enet_peer_send(mPeer, 0, packet);

    /* One could just use enet_host_service() instead. */
    enet_host_flush(mHost.get());
}

void ENetClient::Disconnect()
{
    if (mPeer != nullptr) {
        enet_peer_disconnect(mPeer, 0);
        mDiscTimerStart.emplace(clock_type::now());
    }
}

void ENetClient::ForceDisconnect()
{
    if (mPeer != nullptr) {
        INFO("forcing disconnect");
        enet_peer_reset(mPeer);
    }
}

// Should not use registry here, the client is consuming events in separate thread,
// receiving only events that should be sent to the server
void ENetClient::ConsumeEvents(Registry* aRegistry)
{
    if (mDiscTimerStart.has_value()) {
        if (clock_type::now() - mDiscTimerStart.value() > 3s) {
            ForceDisconnect();
            mConnected = false;
            mRunning   = false;
        }
    }

    EventVisitor visitor{[&](const CreepSpawnEvent& aEvent) {
        INFO("received creep spawn ev");
        std::string evStr("creep_spawn");
        send(evStr);
    }};
    NetEvent*    ev = nullptr;
    while ((ev = mQueue.pop())) {
        std::visit(visitor, *ev);
    }
}

void ENetClient::OnConnect(ENetEvent& aEvent) { mConnected = true; }

void ENetClient::OnReceive(ENetEvent& aEvent, bx::SpScUnboundedQueueT<NetEvent>& aQueue) {}

void ENetClient::OnDisconnect(ENetEvent& aEvent)
{
    mConnected = false;
    mRunning   = false;
}

void ENetClient::OnDisconnectTimeout(ENetEvent& aEvent) {}

void ENetClient::OnNone(ENetEvent& aEvent) {}
