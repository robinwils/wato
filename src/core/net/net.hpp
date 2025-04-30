#pragma once

#include <enet.h>

#include <memory>
#include <variant>

#include "input/action.hpp"

struct ENetHostDeleter {
    void operator()(ENetHost* aHost) const noexcept
    {
        if (aHost) {
            enet_host_destroy(aHost);
        }
    }
};
using enet_host_ptr = std::unique_ptr<ENetHost, ENetHostDeleter>;

struct NewGamePayload {
};

// Events are CRTP classes so there is a concrete type underneath, we need
// to use a variant to represent different event possibilities
using NetworkEventPayload = std::variant<PlayerActions, NewGamePayload>;

enum class PacketType {
    Actions,
    NewGame,
};

struct NetworkEvent {
    PacketType          Type;
    NetworkEventPayload Payload;
};

template <class... Ts>
struct EventVisitor : Ts... {
    using Ts::operator()...;
};
