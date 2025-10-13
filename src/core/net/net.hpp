#pragma once

#include <enet.h>

#include <memory>
#include <variant>

#include "components/player.hpp"
#include "core/state.hpp"
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

struct ClientSyncRequest {
    GameInstanceID GameID;
    GameState      State;

    constexpr static auto Serialize(auto& aArchive, const auto& aSelf)
    {
        aArchive.template Write<GameInstanceID>(&aSelf.GameID, 1);
        GameState::Serialize(aArchive, aSelf.State);
    }

    constexpr static auto Deserialize(auto& aArchive, auto& aSelf)
    {
        aArchive.template Read<GameInstanceID>(&aSelf.GameID, 1);
        GameState::Deserialize(aArchive, aSelf.State);
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

using NetworkRequestPayload  = std::variant<ClientSyncRequest, NewGameRequest>;
using NetworkResponsePayload = std::variant<NewGameResponse, ConnectedResponse>;

enum class PacketType {
    ClientSync,
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

template <>
struct fmt::formatter<ENetAddress> : fmt::formatter<std::string> {
    auto format(ENetAddress aObj, format_context& aCtx) const -> decltype(aCtx.out())
    {
        char host[INET6_ADDRSTRLEN] = {0};

        if (enet_address_get_host(&aObj, host, INET6_ADDRSTRLEN) != 0) {
            return fmt::format_to(aCtx.out(), "(null):{}", aObj.port);
        } else {
            return fmt::format_to(aCtx.out(), "{}:{}", host, aObj.port);
        }
    }
};
