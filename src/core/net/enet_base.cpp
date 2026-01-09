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

bool ENetBase::Send(ENetPeer* aPeer, const std::span<uint8_t> aData)
{
    if (aPeer == nullptr) {
        mLogger->debug("client peer not initialized");
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
                if (event.peer) {
                    mLogger->info("New {}.\n", *event.peer);
                }
                /* Store any relevant client information here. */
                OnConnect(event);
                break;

            case ENET_EVENT_TYPE_RECEIVE: {
                std::string userData   = "(null)";
                std::size_t packetSize = 0;
                if (event.packet && event.packet->data) {
                    userData = std::string(
                        reinterpret_cast<char*>(event.packet->data),
                        event.packet->dataLength);
                    packetSize = event.packet->dataLength;
                }
                std::string peerData = "(null)";
                if (event.peer && event.peer->data) {
                    peerData = std::string(reinterpret_cast<char*>(event.peer->data));
                }

                mLogger->trace(
                    "A packet of length {} was received from {} with data "
                    "{} on "
                    "channel {}.",
                    packetSize,
                    *event.peer,
                    peerData,
                    event.channelID);
                OnReceive(event);

                /* Clean up the packet now that we're done using it. */
                enet_packet_destroy(event.packet);
                break;
            }

            case ENET_EVENT_TYPE_DISCONNECT:
                if (event.peer) {
                    mLogger->info("{} disconnected.\n", *event.peer);
                }
                OnDisconnect(event);
                /* Reset the peer's client information. */
                event.peer->data = NULL;
                break;

            case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
                mLogger->info(
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
