#include "core/net/enet_server.hpp"

#include <bx/spscqueue.h>
#include <enet.h>

#include <stdexcept>

#include "components/creep.hpp"
#include "components/creep_spawn.hpp"
#include "components/health.hpp"
#include "core/event/creep_spawn.hpp"
#include "core/net/net.hpp"
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

void ENetServer::ConsumeEvents(Registry* aRegistry)
{
    EventVisitor visitor{[&](const CreepSpawnEvent& aEvent) {
        INFO("received creep spawn ev");
        auto creepSpawn = aRegistry->create();
        aRegistry->emplace<CreepSpawn>(creepSpawn);
    }};
    NetEvent*    ev = nullptr;
    while ((ev = mQueue.pop())) {
        std::visit(visitor, *ev);
    }
}

void ENetServer::OnConnect(ENetEvent& aEvent) {}

void ENetServer::OnReceive(ENetEvent& aEvent) { mQueue.push(new NetEvent(CreepSpawnEvent())); }

void ENetServer::OnDisconnect(ENetEvent& aEvent) {}

void ENetServer::OnDisconnectTimeout(ENetEvent& aEvent) {}

void ENetServer::OnNone(ENetEvent& aEvent) {}
