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

inline bool operator==(const NewGameRequest& aLHS, const NewGameRequest& aRHS)
{
    return aLHS.PlayerAID == aRHS.PlayerAID;
}

struct SyncPayload {
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

inline bool operator==(const SyncPayload& aLHS, const SyncPayload& aRHS)
{
    return aLHS.GameID == aRHS.GameID && aLHS.State == aRHS.State;
}

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

inline bool operator==(const NewGameResponse& aLHS, const NewGameResponse& aRHS)
{
    return aLHS.GameID == aRHS.GameID;
}

struct ConnectedResponse {
    constexpr static auto Serialize(auto&, const auto&) {}
    constexpr static auto Deserialize(auto&, auto&) { return true; }
};

inline bool operator==(const ConnectedResponse&, const ConnectedResponse&) { return true; }

enum class PacketType {
    ClientSync,
    ServerSync,
    NewGame,
    Connected,
};

using NetworkRequestPayload = std::variant<std::monostate, SyncPayload, NewGameRequest>;
using NetworkResponsePayload =
    std::variant<std::monostate, NewGameResponse, ConnectedResponse, SyncPayload>;

constexpr auto Serialize(auto& aArchive, const NetworkRequestPayload& aObj)
{
    std::visit(
        VariantVisitor{
            [&](const SyncPayload& aSync) { SyncPayload::Serialize(aArchive, aSync); },
            [&](const NewGameRequest& aReq) { NewGameRequest::Serialize(aArchive, aReq); },
            [&](const std::monostate) {},
        },
        aObj);
}

constexpr auto Deserialize(auto& aArchive, NetworkRequestPayload& aReqPl, PacketType aType)
{
    switch (aType) {
        case PacketType::ClientSync:
        case PacketType::ServerSync: {
            SyncPayload req;
            SyncPayload::Deserialize(aArchive, req);
            aReqPl = req;
            break;
        }
        case PacketType::NewGame: {
            NewGameRequest req;
            NewGameRequest::Deserialize(aArchive, req);
            aReqPl = req;
            break;
        }
        default:
            return false;
    }
    return true;
}

constexpr auto Serialize(auto& aArchive, const NetworkResponsePayload& aObj)
{
    std::visit(
        VariantVisitor{
            [&](const SyncPayload& aSync) { SyncPayload::Serialize(aArchive, aSync); },
            [&](const NewGameResponse& aReq) { NewGameResponse::Serialize(aArchive, aReq); },
            [&](const ConnectedResponse& aReq) { ConnectedResponse::Serialize(aArchive, aReq); },
            [&](const std::monostate) {},
        },
        aObj);
}

constexpr auto Deserialize(auto& aArchive, NetworkResponsePayload& aRespPl, PacketType aType)
{
    switch (aType) {
        case PacketType::NewGame: {
            NewGameResponse resp;
            NewGameResponse::Deserialize(aArchive, resp);
            aRespPl = resp;
            break;
        }
        case PacketType::Connected: {
            aRespPl = ConnectedResponse{};
            break;
        }
        case PacketType::ServerSync: {
            SyncPayload resp;
            SyncPayload::Deserialize(aArchive, resp);
            aRespPl = resp;
            break;
        }
        default:
            return false;
    }
    return true;
}

template <typename _Payload>
struct NetworkEvent {
    PacketType Type;
    ::PlayerID PlayerID;
    _Payload   Payload;

    constexpr static auto Serialize(auto& aArchive, NetworkEvent<_Payload>* aSelf)
    {
        aArchive.template Write<PacketType>(&aSelf->Type, sizeof(aSelf->Type));
        aArchive.template Write<::PlayerID>(&aSelf->PlayerID, sizeof(aSelf->PlayerID));
        ::Serialize(aArchive, aSelf->Payload);
    }
    constexpr static auto Deserialize(auto& aArchive, NetworkEvent<_Payload>* aSelf)
    {
        aArchive.template Read<PacketType>(&aSelf->Type, sizeof(PacketType));
        aArchive.template Read<::PlayerID>(&aSelf->PlayerID, sizeof(aSelf->PlayerID));
        ::Deserialize(aArchive, aSelf->Payload, aSelf->Type);
    }
};

using NetworkResponse = NetworkEvent<NetworkResponsePayload>;
using NetworkRequest  = NetworkEvent<NetworkRequestPayload>;

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

template <>
struct fmt::formatter<ENetPeer> : fmt::formatter<std::string> {
    auto format(ENetPeer aObj, format_context& aCtx) const -> decltype(aCtx.out())
    {
        return fmt::format_to(aCtx.out(), "peer {} @ {}", enet_peer_get_id(&aObj), aObj.address);
    }
};

#include "core/snapshot.hpp"
#include "doctest.h"

TEST_CASE("net.serialize")
{
    std::vector<uint8_t> storage;
    ByteOutputArchive    outAr(storage);
    auto*                ev = new NetworkRequest;

    ev->PlayerID = 42;
    ev->Type     = PacketType::ClientSync;
    ev->Payload  = SyncPayload{.GameID = 21, .State = GameState{.Tick = 12}};

    NetworkRequest::Serialize(outAr, ev);

    ByteInputArchive inAr(std::span(outAr.Bytes()));
    auto*            ev2 = new NetworkRequest;

    NetworkRequest::Deserialize(inAr, ev2);
    CHECK_EQ(ev->PlayerID, ev2->PlayerID);
    CHECK_EQ(ev->Type, ev2->Type);
    CHECK_EQ(ev->Payload, ev2->Payload);
}
