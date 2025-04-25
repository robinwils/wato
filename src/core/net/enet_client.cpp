#include "core/net/enet_client.hpp"

#include <bx/string.h>
#include <enet.h>

#include <stdexcept>
#include <variant>

#include "core/event/creep_spawn.hpp"
#include "core/net/net.hpp"
#include "core/sys/log.hpp"

using namespace std::literals::chrono_literals;

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

void ENetClient::send(std::string& aEvStr)
{
    if (mPeer == nullptr) {
        DBG("client peer not initialized");
        return;
    }
    /* Create a reliable packet of size 7 containing "packet\0" */
    ENetPacket* packet =
        enet_packet_create(aEvStr.c_str(), aEvStr.size(), ENET_PACKET_FLAG_RELIABLE);

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

void ENetClient::OnReceive(ENetEvent& aEvent) {}

void ENetClient::OnDisconnect(ENetEvent& aEvent)
{
    mConnected = false;
    mRunning   = false;
}

void ENetClient::OnDisconnectTimeout(ENetEvent& aEvent) {}

void ENetClient::OnNone(ENetEvent& aEvent) {}
