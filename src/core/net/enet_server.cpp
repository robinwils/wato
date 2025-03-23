#include "core/net/enet_server.hpp"

#include <enet.h>

#include <stdexcept>

ENetServer::~ENetServer() { enet_deinitialize(); }

void ENetServer::Init()
{
    if (enet_initialize() != 0) {
        throw std::runtime_error("failed to initialize Enet");
    }

    // Bind the server to the default localhost.
    ENetAddress address = {.host = ENET_HOST_ANY, .port = 7777, .sin6_scope_id = 0};

    mServer = enet_host_ptr{enet_host_create(&address, 128, 2, 0, 0)};

    if (mServer == nullptr) {
        throw std::runtime_error("failed to initialize Enet");
    }
}

void ENetServer::Run()
{
    if (!mServer) {
        throw std::runtime_error("server is not initialized");
    }
    ENetEvent event;

    /* Wait up to x milliseconds for an event. (WARNING: blocking) */
    while (enet_host_service(mServer.get(), &event, 10000) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                printf("A new client connected from %x:%u.\n",
                    event.peer->address.host,
                    event.peer->address.port);
                /* Store any relevant client information here. */
                event.peer->data = (void *)"Client information";
                break;

            case ENET_EVENT_TYPE_RECEIVE:
                printf("A packet of length %lu containing %s was received from %s on channel %u.\n",
                    event.packet->dataLength,
                    event.packet->data,
                    static_cast<char *>(event.peer->data),
                    event.channelID);
                /* Clean up the packet now that we're done using it. */
                enet_packet_destroy(event.packet);
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                printf("%s disconnected.\n", (char *)event.peer->data);
                /* Reset the peer's client information. */
                event.peer->data = NULL;
                break;

            case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
                printf("%s disconnected due to timeout.\n", (char *)event.peer->data);
                /* Reset the peer's client information. */
                event.peer->data = NULL;
                break;

            case ENET_EVENT_TYPE_NONE:
                break;
        }
    }
}
