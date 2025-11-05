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

    bool Archive(auto& aArchive)
    {
        if (!ArchiveValue(aArchive, PlayerAID, 0u, 1000000000u)) return false;
        return true;
    }

    constexpr static auto Serialize(auto& aArchive, const auto& aSelf)
    {
        ::Serialize(aArchive, aSelf.PlayerAID);
    }
    constexpr static auto Deserialize(auto& aArchive, auto& aSelf)
    {
        ::Deserialize(aArchive, aSelf.PlayerAID);
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

    bool Archive(auto& aArchive)
    {
        if (!ArchiveValue(aArchive, GameID, 0ul, std::numeric_limits<uint64_t>::max()))
            return false;
        if (!State.Archive(aArchive)) return false;
        return true;
    }

    constexpr static auto Serialize(auto& aArchive, const auto& aSelf)
    {
        ::Serialize(aArchive, aSelf.GameID);
        GameState::Serialize(aArchive, aSelf.State);
    }

    constexpr static auto Deserialize(auto& aArchive, auto& aSelf)
    {
        ::Deserialize(aArchive, aSelf.GameID);
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

    bool Archive(auto& aArchive)
    {
        if (!ArchiveValue(aArchive, GameID, 0u, std::numeric_limits<uint64_t>::max())) return false;
        return true;
    }

    constexpr static auto Serialize(auto& aArchive, const auto& aSelf)
    {
        ::Serialize(aArchive, aSelf.GameID);
    }
    constexpr static auto Deserialize(auto& aArchive, auto& aSelf)
    {
        ::Deserialize(aArchive, aSelf.GameID);
        return true;
    }
};

inline bool operator==(const NewGameResponse& aLHS, const NewGameResponse& aRHS)
{
    return aLHS.GameID == aRHS.GameID;
}

struct ConnectedResponse {
    bool                  Archive(auto& aArchive) { return true; }
    constexpr static auto Serialize(auto&, const auto&) {}
    constexpr static auto Deserialize(auto&, auto&) { return true; }
};

inline bool operator==(const ConnectedResponse&, const ConnectedResponse&) { return true; }

struct TowerValidationResponse {
    bool         Valid;
    entt::entity Entity;

    bool                  Archive(auto& aArchive) { return true; }
    constexpr static auto Serialize(auto&, const auto&) {}
    constexpr static auto Deserialize(auto&, auto&) { return true; }
};

inline bool operator==(const TowerValidationResponse& aLHS, const TowerValidationResponse& aRHS)
{
    return aLHS.Valid == aRHS.Valid && aLHS.Entity == aRHS.Entity;
}

enum class PacketType : std::uint16_t {
    ClientSync,
    ServerSync,
    TowerValidation,
    NewGame,
    Connected,
    Count,
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

    bool Archive(auto& aArchive)
    {
        if (!ArchiveValue(aArchive, Type, 0u, uint32_t(PacketType::Count))) return false;
        if (!ArchiveValue(aArchive, PlayerID, 0u, 1000000000u)) return false;
        if (!ArchiveVariant(aArchive, Payload)) return false;
        return true;
    }

    constexpr static auto Serialize(auto& aArchive, NetworkEvent<_Payload>* aSelf)
    {
        ::Serialize(aArchive, aSelf->Type);
        ::Serialize(aArchive, aSelf->PlayerID);
        ::Serialize(aArchive, aSelf->Payload);
    }
    constexpr static auto Deserialize(auto& aArchive, NetworkEvent<_Payload>* aSelf)
    {
        ::Deserialize(aArchive, aSelf->Type);
        ::Deserialize(aArchive, aSelf->PlayerID);
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

#ifndef DOCTEST_CONFIG_DISABLE
#include "core/snapshot.hpp"
#include "test.hpp"

TEST_CASE("net.serialize")
{
    BitOutputArchive outAr;
    auto*            ev = new NetworkRequest;

    std::uint64_t gameID =
        (static_cast<std::uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count())
         << 32)
        | 42ul;

    ev->PlayerID = 42;
    ev->Type     = PacketType::ClientSync;
    ev->Payload  = SyncPayload{.GameID = gameID, .State = GameState{.Tick = 12}};

    ev->Archive(outAr);

    BitInputArchive inAr(outAr.Data());
    auto*           ev2 = new NetworkRequest;

    ev2->Archive(inAr);

    CHECK_EQ(ev->PlayerID, ev2->PlayerID);
    CHECK_EQ(ev->Type, ev2->Type);
    CHECK_EQ(ev->Payload, ev2->Payload);
    delete ev;
    delete ev2;
}
#endif
