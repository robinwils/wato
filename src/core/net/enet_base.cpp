#include "core/net/enet_base.hpp"

#include <enet.h>
#include <sodium.h>

#include <stdexcept>
#include <string>

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

void ENetBase::Poll()
{
    if (!mHost) {
        throw std::runtime_error("host is not initialized");
    }
    ENetEvent event;

    /* Wait up to x milliseconds for an event. (WARNING: blocking) */
    while (enet_host_service(mHost.get(), &event, 5) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                INFO("A new client connected from %x:%u.\n",
                    event.peer->address.host,
                    event.peer->address.port);
                /* Store any relevant client information here. */
                OnConnect(event);
                break;

            case ENET_EVENT_TYPE_RECEIVE: {
                std::string str(reinterpret_cast<char*>(event.packet->data),
                    event.packet->dataLength);
                INFO(
                    "A packet of length %lu containing %s was received from %s on "
                    "channel %u.\n",
                    str.size(),
                    str.c_str(),
                    static_cast<char*>(event.peer->data),
                    event.channelID);
                OnReceive(event);

                /* Clean up the packet now that we're done using it. */
                enet_packet_destroy(event.packet);
                break;
            }

            case ENET_EVENT_TYPE_DISCONNECT:
                INFO("%s disconnected.\n", (char*)event.peer->data);
                OnDisconnect(event);
                /* Reset the peer's client information. */
                event.peer->data = NULL;
                break;

            case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
                INFO("%s disconnected due to timeout.\n", (char*)event.peer->data);
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
