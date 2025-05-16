#pragma once

#include <enet.h>

#include <memory>
#include <variant>

#include "components/player.hpp"
#include "core/types.hpp"
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

struct NewGameRequest {
    PlayerID PlayerAID;

    constexpr static auto Serialize(auto& aArchive, const auto& aSelf)
    {
        aArchive.template Write<PlayerID>(&aSelf.PlayerAID, 1);
    }
    constexpr static auto Deserialize(auto& aArchive, auto& aSelf)
    {
        aArchive.template Read<PlayerID>(&aSelf.PlayerAID, 1);
        return true;
    }
};

struct NewGameResponse {
    GameInstanceID GameID;

    constexpr static auto Serialize(auto& aArchive, const auto& aSelf)
    {
        aArchive.template Write<GameInstanceID>(&aSelf.GameID, 1);
    }
    constexpr static auto Deserialize(auto& aArchive, auto& aSelf)
    {
        aArchive.template Read<GameInstanceID>(&aSelf.GameID, 1);
        return true;
    }
};

struct ConnectedResponse {
};

using NetworkRequestPayload  = std::variant<PlayerActions, NewGameRequest>;
using NetworkResponsePayload = std::variant<NewGameResponse, ConnectedResponse>;

enum class PacketType {
    Actions,
    NewGame,
    Connected,
};

template <typename _Payload>
struct NetworkEvent {
    PacketType Type;
    ::PlayerID PlayerID;
    _Payload   Payload;
};

template struct NetworkEvent<NetworkRequestPayload>;
template struct NetworkEvent<NetworkResponsePayload>;
