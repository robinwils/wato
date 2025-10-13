#include "core/net/enet_base.hpp"

#include <enet.h>
#include <sodium.h>
#include <spdlog/spdlog.h>

#include <stdexcept>

#include "core/net/net.hpp"
#include "core/sys/log.hpp"

ENetBase::~ENetBase() { enet_deinitialize(); }

void ENetBase::Init()
{
    if (enet_initialize() != 0) {
        throw std::runtime_error("failed to initialize Enet");
    }

    if (sodium_init() == -1) {
        throw std::runtime_error("failed to initialize Sodium");
    }
}

bool ENetBase::Send(ENetPeer* aPeer, const std::vector<uint8_t> aData)
{
    if (aPeer == nullptr) {
        WATO_DBG("client peer not initialized");
        return false;
    }

    ENetPacket* packet = enet_packet_create(aData.data(), aData.size(), ENET_PACKET_FLAG_RELIABLE);

    if (-1 == enet_peer_send(aPeer, 0, packet)) {
        enet_packet_destroy(packet);
        return false;
    }

    enet_host_flush(mHost.get());
    return true;
}

void ENetBase::Poll()
{
    // TODO: not propagated if thrown in thread
    if (!mHost) {
        throw std::runtime_error("host is not initialized");
    }
    ENetEvent event;

    /* Wait up to x milliseconds for an event. (WARNING: blocking) */
    while (enet_host_service(mHost.get(), &event, 5) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                spdlog::info("A new client connected from {}.\n", event.peer->address);
                /* Store any relevant client information here. */
                OnConnect(event);
                break;

            case ENET_EVENT_TYPE_RECEIVE: {
                std::string userData = "(null)";
                if (event.packet->data) {
                    userData = std::string(
                        reinterpret_cast<char*>(event.packet->data),
                        event.packet->dataLength);
                }
                std::string peerData = "(null)";
                if (event.peer->data) {
                    peerData = std::string(reinterpret_cast<char*>(event.peer->data));
                }
                spdlog::debug(
                    "A packet of length {} was received from peer ID {} with data "
                    "{} on "
                    "channel {}.",
                    event.packet->dataLength,
                    enet_peer_get_id(event.peer),
                    peerData,
                    event.channelID);
                OnReceive(event);

                /* Clean up the packet now that we're done using it. */
                enet_packet_destroy(event.packet);
                break;
            }

            case ENET_EVENT_TYPE_DISCONNECT:
                spdlog::info(
                    "{} disconnected.\n",
                    event.peer->data ? static_cast<char*>(event.peer->data) : "?");
                OnDisconnect(event);
                /* Reset the peer's client information. */
                event.peer->data = NULL;
                break;

            case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
                spdlog::info(
                    "{} disconnected due to timeout.\n",
                    static_cast<char*>(event.peer->data));
                /* Reset the peer's client information. */
                OnDisconnectTimeout(event);
                event.peer->data = NULL;
                break;

            case ENET_EVENT_TYPE_NONE:
                OnNone(event);
                break;
        }
    }
}
