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
        if (!ArchiveValue(aArchive, GameID, uint64_t(0), std::numeric_limits<uint64_t>::max()))
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
    entt::entity   PlayerEntity;

    bool Archive(auto& aArchive)
    {
        if (!ArchiveValue(aArchive, GameID, uint64_t(0), std::numeric_limits<uint64_t>::max()))
            return false;
        return ArchiveEntity(aArchive, PlayerEntity);
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

enum class RigidBodyEvent : std::uint16_t {
    Create,
    Update,
    Destroy,
};

template <>
struct fmt::formatter<RigidBodyEvent> : fmt::formatter<std::string> {
    auto format(RigidBodyEvent const& aObj, format_context& aCtx) const -> decltype(aCtx.out())
    {
        switch (aObj) {
            case RigidBodyEvent::Create:
                return fmt::format_to(aCtx.out(), "Create Event");
            case RigidBodyEvent::Update:
                return fmt::format_to(aCtx.out(), "Update Event");
            case RigidBodyEvent::Destroy:
                return fmt::format_to(aCtx.out(), "Destroy Event");
            default:
                return fmt::format_to(aCtx.out(), "Unknown Event");
        }
    }
};

struct ProjectileInitData {
    entt::entity SourceTower;
    float        Damage;
    float        Speed;
    entt::entity Target;

    bool Archive(auto& aArchive)
    {
        if (!ArchiveEntity(aArchive, SourceTower)) return false;
        if (!ArchiveValue(aArchive, Damage, 0.0f, 100.0f)) return false;
        if (!ArchiveValue(aArchive, Speed, 0.0f, 10.0f)) return false;
        if (!ArchiveEntity(aArchive, Target)) return false;
        return true;
    }
};

inline bool operator==(const ProjectileInitData& aLHS, const ProjectileInitData& aRHS)
{
    return aLHS.SourceTower == aRHS.SourceTower && aLHS.Damage == aRHS.Damage
           && aLHS.Speed == aRHS.Speed && aLHS.Target == aRHS.Target;
}

struct TowerInitData {
    TowerType Type;
    glm::vec3 Position;

    bool Archive(auto& aArchive)
    {
        if (!ArchiveValue(aArchive, Type, 0u, uint32_t(TowerType::Count))) return false;
        if (!ArchiveVector(aArchive, Position, 0.0f, 20.0f)) return false;
        return true;
    }
};

inline bool operator==(const TowerInitData& aLHS, const TowerInitData& aRHS)
{
    return aLHS.Type == aRHS.Type && aLHS.Position == aRHS.Position;
}

struct CreepInitData {
    CreepType Type;
    glm::vec3 Position;

    bool Archive(auto& aArchive)
    {
        if (!ArchiveValue(aArchive, Type, 0u, uint32_t(CreepType::Count))) return false;
        if (!ArchiveVector(aArchive, Position, 0.0f, 20.0f)) return false;
        return true;
    }
};

inline bool operator==(const CreepInitData& aLHS, const CreepInitData& aRHS)
{
    return aLHS.Type == aRHS.Type && aLHS.Position == aRHS.Position;
}

using EntityInitData =
    std::variant<std::monostate, ProjectileInitData, TowerInitData, CreepInitData>;

template <>
struct fmt::formatter<EntityInitData> : fmt::formatter<std::string> {
    auto format(EntityInitData const& aObj, format_context& aCtx) const -> decltype(aCtx.out())
    {
        std::visit(
            VariantVisitor{
                [&](const ProjectileInitData&) {
                    fmt::format_to(aCtx.out(), "projectile init data");
                },
                [&](const TowerInitData&) { fmt::format_to(aCtx.out(), "tower init data"); },
                [&](const CreepInitData&) { fmt::format_to(aCtx.out(), "creep init data"); },
                [&](const std::monostate&) { fmt::format_to(aCtx.out(), "no init data"); },
            },
            aObj);
        return aCtx.out();
    }
};

struct RigidBodyUpdateResponse {
    RigidBodyParams Params;
    entt::entity    Entity;
    RigidBodyEvent  Event;
    EntityInitData  InitData;

    bool Archive(auto& aArchive)
    {
        if (!Params.Archive(aArchive)) return false;
        if (!ArchiveEntity(aArchive, Entity)) return false;
        if (!ArchiveValue(aArchive, Event, uint16_t(0), uint16_t(RigidBodyEvent::Destroy)))
            return false;
        if (!ArchiveVariant(aArchive, InitData)) return false;
        return true;
    }
};

inline bool operator==(const RigidBodyUpdateResponse& aLHS, const RigidBodyUpdateResponse& aRHS)
{
    return aLHS.Params == aRHS.Params && aLHS.Entity == aRHS.Entity && aLHS.Event == aRHS.Event
           && aLHS.InitData == aRHS.InitData;
}

struct HealthUpdateResponse {
    entt::entity Entity;
    float        Health;

    bool Archive(auto& aArchive)
    {
        if (!ArchiveEntity(aArchive, Entity)) return false;
        return ArchiveValue(aArchive, Health, 0.0f, 1000.0f);
    }
};

inline bool operator==(const HealthUpdateResponse& aLHS, const HealthUpdateResponse& aRHS)
{
    return aLHS.Entity == aRHS.Entity && aLHS.Health == aRHS.Health;
}

struct PlayerEliminatedResponse {
    ::PlayerID              PlayerID;
    std::vector<::PlayerID> Ranking;

    bool Archive(auto& aArchive)
    {
        if (!ArchiveValue(aArchive, PlayerID, 0u, 1000000000u)) return false;
        return ArchiveVector(
            aArchive,
            Ranking,
            ::PlayerID(0),
            ::PlayerID(std::numeric_limits<::PlayerID>::max()),
            8u);
    }
};

inline bool operator==(const PlayerEliminatedResponse& aLHS, const PlayerEliminatedResponse& aRHS)
{
    return aLHS.PlayerID == aRHS.PlayerID && aLHS.Ranking == aRHS.Ranking;
}

struct GameEndResponse {
    std::vector<PlayerID> Ranking;

    bool Archive(auto& aArchive)
    {
        return ArchiveVector(
            aArchive,
            Ranking,
            PlayerID(0),
            PlayerID(std::numeric_limits<PlayerID>::max()),
            8u);
    }
};

inline bool operator==(const GameEndResponse& aLHS, const GameEndResponse& aRHS)
{
    return aLHS.Ranking == aRHS.Ranking;
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
    RigidBodyUpdateResponse,
    HealthUpdateResponse,
    PlayerEliminatedResponse,
    GameEndResponse>;

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
struct fmt::formatter<NetworkResponsePayload> : fmt::formatter<std::string> {
    auto format(NetworkResponsePayload const& aObj, format_context& aCtx) const
        -> decltype(aCtx.out())
    {
        std::visit(
            VariantVisitor{
                [&](const NewGameResponse& aResp) {
                    fmt::format_to(aCtx.out(), "new game response, game ID {}", aResp.GameID);
                },
                [&](const ConnectedResponse&) { fmt::format_to(aCtx.out(), "connected response"); },
                [&](const SyncPayload& aResp) {
                    fmt::format_to(aCtx.out(), "sync payload for game {}", aResp.GameID);
                },
                [&](const RigidBodyUpdateResponse& aResp) {
                    fmt::format_to(
                        aCtx.out(),
                        "rigid body update, {}, with {}",
                        aResp.Event,
                        aResp.InitData);
                },
                [&](const HealthUpdateResponse& aResp) {
                    fmt::format_to(
                        aCtx.out(),
                        "health update for {}: {}",
                        aResp.Entity,
                        aResp.Health);
                },
                [&](const PlayerEliminatedResponse& aResp) {
                    fmt::format_to(
                        aCtx.out(),
                        "player {} eliminated, current rankings: {}",
                        aResp.PlayerID,
                        aResp.Ranking);
                },
                [&](const GameEndResponse& aResp) {
                    fmt::format_to(aCtx.out(), "game ended, final rankings: {}", aResp.Ranking);
                },
                [&](const std::monostate&) {
                    fmt::format_to(aCtx.out(), "no network response payload");
                },
            },
            aObj);
        return aCtx.out();
    }
};

template <typename PL>
struct fmt::formatter<NetworkEvent<PL>> : fmt::formatter<std::string> {
    auto format(NetworkEvent<PL> const& aObj, format_context& aCtx) const -> decltype(aCtx.out())
    {
        return fmt::format_to(
            aCtx.out(),
            "network event at tick {} for player {}: {}",
            aObj.Tick,
            aObj.PlayerID,
            aObj.Payload);
    }
};

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
