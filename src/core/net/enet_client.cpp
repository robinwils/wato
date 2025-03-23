#include "core/net/enet_client.hpp"

#include <enet.h>

#include <stdexcept>

ENetClient::~ENetClient() { enet_deinitialize(); }

void ENetClient::Init()
{
    if (enet_initialize() != 0) {
        throw std::runtime_error("failed to initialize Enet");
    }

    // Bind the server to the default localhost.
    ENetAddress address = {.host = ENET_HOST_ANY, .port = 7777, .sin6_scope_id = 0};

    mClient = enet_host_ptr{enet_host_create(&address, 128, 2, 0, 0)};

    if (mClient == nullptr) {
        throw std::runtime_error("failed to initialize Enet");
    }
}

void ENetClient::Run()
{
    if (!mClient) {
        throw std::runtime_error("server is not initialized");
    }
}
