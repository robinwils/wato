#pragma once

#include <enet.h>

#include <memory>
#include <variant>

#include "components/player.hpp"
#include "core/physics/physics.hpp"
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
};

inline bool operator==(const SyncPayload& aLHS, const SyncPayload& aRHS)
{
    return aLHS.GameID == aRHS.GameID && aLHS.State == aRHS.State;
}

struct NewGameResponse {
    GameInstanceID GameID;

    bool Archive(auto& aArchive)
    {
        if (!ArchiveValue(aArchive, GameID, 0ul, std::numeric_limits<uint64_t>::max()))
            return false;
        return true;
    }
};

inline bool operator==(const NewGameResponse& aLHS, const NewGameResponse& aRHS)
{
    return aLHS.GameID == aRHS.GameID;
}

struct ConnectedResponse {
    bool Archive(auto&) { return true; }
};

inline bool operator==(const ConnectedResponse&, const ConnectedResponse&) { return true; }

struct AcknowledgementResponse {
    bool         Ack;
    entt::entity Entity, ServerEntity;

    bool Archive(auto& aArchive)
    {
        if (!ArchiveBool(aArchive, Ack)) return false;
        if (!ArchiveEntity(aArchive, Entity)) return false;
        return ArchiveEntity(aArchive, ServerEntity);
    }
};

inline bool operator==(const AcknowledgementResponse& aLHS, const AcknowledgementResponse& aRHS)
{
    return aLHS.Ack == aRHS.Ack && aLHS.Entity == aRHS.Entity
           && aLHS.ServerEntity == aRHS.ServerEntity;
}

struct RigidBodyUpdateResponse {
    RigidBodyParams Params;
    entt::entity    Entity;

    bool Archive(auto& aArchive)
    {
        if (!Params.Archive(aArchive)) return false;
        return ArchiveEntity(aArchive, Entity);
    }
};

inline bool operator==(const RigidBodyUpdateResponse& aLHS, const RigidBodyUpdateResponse& aRHS)
{
    return aLHS.Params == aRHS.Params && aLHS.Entity == aRHS.Entity;
}

enum class PacketType : std::uint16_t {
    ClientSync,
    ServerSync,
    Ack,
    NewGame,
    Connected,
    Count,
};

using NetworkRequestPayload  = std::variant<std::monostate, SyncPayload, NewGameRequest>;
using NetworkResponsePayload = std::variant<
    std::monostate,
    NewGameResponse,
    ConnectedResponse,
    SyncPayload,
    AcknowledgementResponse,
    RigidBodyUpdateResponse>;

template <typename _Payload>
struct NetworkEvent {
    PacketType Type;
    ::PlayerID PlayerID;
    uint32_t   Tick;
    _Payload   Payload;

    bool Archive(auto& aArchive)
    {
        if (!ArchiveValue(aArchive, Type, 0u, uint32_t(PacketType::Count))) return false;
        if (!ArchiveValue(aArchive, PlayerID, 0u, 1000000000u)) return false;
        if (!ArchiveValue(aArchive, Tick, 0u, 30000000u)) return false;
        if (!ArchiveVariant(aArchive, Payload)) return false;
        return true;
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
    ev->Tick     = 0;
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
