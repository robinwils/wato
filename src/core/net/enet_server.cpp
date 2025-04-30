#include "core/net/enet_server.hpp"

#include <bx/spscqueue.h>
#include <enet.h>
#include <fmt/base.h>

#include <span>
#include <stdexcept>

#include "core/net/net.hpp"
#include "core/snapshot.hpp"
#include "core/sys/log.hpp"

void ENetServer::Init()
{
    ENetBase::Init();

    // Bind the server to the default localhost.
    ENetAddress address = {.host = ENET_HOST_ANY, .port = 7777, .sin6_scope_id = 0};

    mHost = enet_host_ptr{enet_host_create(&address, 128, 2, 0, 0)};

    if (!mHost) {
        throw std::runtime_error("An error occurred while trying to create an ENet server host.");
    }
}

void ENetServer::OnConnect(ENetEvent& aEvent)
{
    mQueue.push(new NetworkEvent{.Type = PacketType::NewGame, .Payload = NewGamePayload{}});
}

void ENetServer::OnReceive(ENetEvent& aEvent)
{
    std::span<uint8_t>(aEvent.packet->data, aEvent.packet->dataLength);
    ByteInputArchive archive(std::span<uint8_t>(aEvent.packet->data, aEvent.packet->dataLength));
    auto*            ev = new NetworkEvent();

    archive.Read<PacketType>(&ev->Type, sizeof(PacketType));
    switch (ev->Type) {
        case PacketType::Actions: {
            PlayerActions actions;
            PlayerActions::Deserialize(archive, actions);
            ev->Payload = actions;
            break;
        }
        case PacketType::NewGame:
            break;
    }
    mQueue.push(ev);
}

void ENetServer::OnDisconnect(ENetEvent& aEvent) {}

void ENetServer::OnDisconnectTimeout(ENetEvent& aEvent) {}

void ENetServer::OnNone(ENetEvent& aEvent) {}
