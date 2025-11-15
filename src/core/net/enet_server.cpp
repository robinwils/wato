#include "core/net/enet_server.hpp"

#include <bx/spscqueue.h>
#include <enet.h>
#include <spdlog/spdlog.h>

#include <span>
#include <stdexcept>

#include "components/player.hpp"
#include "core/net/net.hpp"
#include "core/snapshot.hpp"
#include "core/sys/log.hpp"

void ENetServer::Init()
{
    ENetBase::Init();

    const std::size_t pos  = mServerAddr.find(':');
    enet_uint16       port = 7777;
    std::string       host = mServerAddr;

    if (pos != std::string::npos) {
        host = mServerAddr.substr(0, pos);
    }

    // Bind the server to the default localhost.
    ENetAddress address = {.host = ENET_HOST_ANY, .port = port, .sin6_scope_id = 0};

    if (host != "any") {
        if (-1 == enet_address_set_host(&address, host.c_str())) {
            throw std::runtime_error(fmt::format(
                "An error occurred while trying to set ENet server host on '{}'",
                host.c_str()));
        }
    }

    mHost = enet_host_ptr{enet_host_create(&address, 128, 2, 0, 0)};

    if (!mHost) {
        throw std::runtime_error("An error occurred while trying to create an ENet server host.");
    }
    mLogger->info("created ENet server at {}:{}", host, port);
}

void ENetServer::OnConnect(ENetEvent&) {}

void ENetServer::OnReceive(ENetEvent& aEvent)
{
    BitInputArchive archive(std::span<uint8_t>(aEvent.packet->data, aEvent.packet->dataLength));
    auto            ev = new NetworkRequest;

    if (!ev->Archive(archive)) {
        spdlog::critical("cannot decode packet");
    }

    if (ev->Type == PacketType::NewGame) {
        auto payload = std::get<NewGameRequest>(ev->Payload);
        if (!mConnectedPeers.contains(payload.PlayerAID)) {
            mConnectedPeers[payload.PlayerAID] = aEvent.peer;
        }
    }
    mReqChannel.Send(ev);
}

void ENetServer::OnDisconnect(ENetEvent&) {}

void ENetServer::OnDisconnectTimeout(ENetEvent&) {}

void ENetServer::OnNone(ENetEvent&) {}
