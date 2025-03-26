#include "core/net/enet_client.hpp"

#include <bx/string.h>
#include <enet.h>

#include <stdexcept>

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
    ENetEvent   event{};

    /* Connect to some.server.net:1234. */
    enet_address_set_host(&address, "127.0.0.1");
    address.port = 7777;

    /* Initiate the connection, allocating the two channels 0 and 1. */
    mPeer = enet_host_connect(mHost.get(), &address, 2, 0);
    if (mPeer == nullptr) {
        return false;
    }

    /* Wait up to 5 seconds for the connection attempt to succeed. */
    if (enet_host_service(mHost.get(), &event, 5000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT) {
        return true;
    } else {
        /* Either the 5 seconds are up or a disconnect event was */
        /* received. Reset the peer in the event the 5 seconds   */
        /* had run out without any significant event.            */
        enet_peer_reset(mPeer);
        return false;
    }
}

void ENetClient::Send()
{
    if (mPeer == nullptr) {
        throw std::runtime_error("client peer not initialized.");
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
    enet_peer_disconnect(mPeer, 0);

    ENetEvent event{};

    uint8_t disconnected = false;
    /* Allow up to 3 seconds for the disconnect to succeed
     * and drop any packets received packets.
     */
    while (enet_host_service(mHost.get(), &event, 3000) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_RECEIVE:
                enet_packet_destroy(event.packet);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                puts("Disconnection succeeded.");
                disconnected = true;
                break;
            default:
                throw std::runtime_error("unhandled event type after disconnection");
        }
    }

    // Drop connection, since disconnection didn't successed
    if (!disconnected) {
        enet_peer_reset(mPeer);
    }
}

void ENetClient::OnConnect(ENetEvent& aEvent) {}

void ENetClient::OnReceive(ENetEvent& aEvent, bx::SpScUnboundedQueueT<NetEvent>& aQueue) {}

void ENetClient::OnDisconnect(ENetEvent& aEvent) {}

void ENetClient::OnDisconnectTimeout(ENetEvent& aEvent) {}

void ENetClient::OnNone(ENetEvent& aEvent) {}
