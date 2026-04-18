#pragma once

#include <enet.h>

#include <memory>
#include <variant>

#include "components/player.hpp"
#include "components/tower_attack.hpp"
#include "core/crypto/key.hpp"
#include "core/physics/physics.hpp"
#include "core/serialize.hpp"
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

struct PlayerInitData {
    PlayerID     ID;
    entt::entity ServerEntity;
    float        Health;
    int          StartingGold{};
    std::string  DisplayName;
    glm::vec3    Position;
    glm::uvec2   MapSize;
    glm::vec2    MapWorldOffset;

    bool Archive(auto& aArchive)
    {
        if (!ArchivePlayerID(aArchive, ID)) return false;
        if (!ArchiveEntity(aArchive, ServerEntity)) return false;
        if (!ArchiveValue(aArchive, Health, -10.0f, 1000.0f)) return false;
        if (!ArchiveValue(aArchive, StartingGold, 0, 100000)) return false;
        if (!ArchiveString(aArchive, DisplayName, 5000)) return false;
        if (!ArchiveVector(aArchive, Position, 0.0f, 500.0f)) return false;
        if (!ArchiveVector(aArchive, MapSize, 0u, 100u)) return false;
        if (!ArchiveVector(aArchive, MapWorldOffset, 0.0f, 500.0f)) return false;
        return true;
    }

    bool operator==(const PlayerInitData&) const = default;
};

struct NewGameResponse {
    GameInstanceID              GameID;
    PlayerID                    YourPlayerID;
    int                         StartingIncome;
    std::vector<PlayerInitData> Players;

    bool Archive(auto& aArchive)
    {
        if (!ArchiveValue(aArchive, GameID, uint64_t(0), std::numeric_limits<uint64_t>::max()))
            return false;
        if (!ArchivePlayerID(aArchive, YourPlayerID)) return false;
        if (!ArchiveValue(aArchive, StartingIncome, 0, 500)) return false;
        return ArchiveVector(aArchive, Players, 8u);
    }

    bool operator==(const NewGameResponse&) const = default;
};

struct ConnectedResponse {
    bool Archive(auto&) { return true; }
};

inline bool operator==(const ConnectedResponse&, const ConnectedResponse&) { return true; }

enum class ServerError : std::uint8_t {
    Success,
    HandshakeOpenSeal,
};

struct ErrorResponse {
    ServerError Error{};

    bool Archive(auto& aArchive)
    {
        return ArchiveValue(aArchive, Error, ServerError::Success, ServerError::HandshakeOpenSeal);
    }

    auto operator<=>(const ErrorResponse&) const = default;
};

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
    entt::entity     SourceTower;
    float            Damage;
    float            Speed;
    entt::entity     Target;
    PlayerID         OwnerID{};
    ::ColliderParams ColliderParams;

    bool Archive(auto& aArchive)
    {
        if (!ArchiveEntity(aArchive, SourceTower)) return false;
        if (!ArchiveValue(aArchive, Damage, 0.0f, 100.0f)) return false;
        if (!ArchiveValue(aArchive, Speed, 0.0f, 10.0f)) return false;
        if (!ArchiveEntity(aArchive, Target)) return false;
        if (!ArchivePlayerID(aArchive, OwnerID)) return false;
        return ColliderParams.Archive(aArchive);
    }
};

inline bool operator==(const ProjectileInitData& aLHS, const ProjectileInitData& aRHS)
{
    return aLHS.SourceTower == aRHS.SourceTower && aLHS.Damage == aRHS.Damage
           && aLHS.Speed == aRHS.Speed && aLHS.Target == aRHS.Target && aLHS.OwnerID == aRHS.OwnerID
           && aLHS.ColliderParams == aRHS.ColliderParams;
}

struct TowerInitData {
    TowerType        Type;
    glm::vec3        Position;
    float            Health{};
    TowerAttack      Attack{};
    PlayerID         OwnerID;
    ::ColliderParams ColliderParams;

    bool Archive(auto& aArchive)
    {
        if (!ArchiveValue(aArchive, Type, 0u, uint32_t(TowerType::Count))) return false;
        if (!ArchiveVector(aArchive, Position, 0.0f, 500.0f)) return false;
        if (!ArchiveValue(aArchive, Health, 0.0f, 1000.0f)) return false;
        if (!Attack.Archive(aArchive)) return false;
        if (!ArchivePlayerID(aArchive, OwnerID)) return false;
        return ColliderParams.Archive(aArchive);
    }
};

inline bool operator==(const TowerInitData& aLHS, const TowerInitData& aRHS)
{
    return aLHS.Type == aRHS.Type && aLHS.Position == aRHS.Position && aLHS.OwnerID == aRHS.OwnerID;
}

struct CreepInitData {
    CreepType        Type;
    glm::vec3        Position;
    float            Health;
    float            Damage;
    PlayerID         OwnerID;
    ::ColliderParams ColliderParams;

    bool Archive(auto& aArchive)
    {
        if (!ArchiveValue(aArchive, Type, 0u, uint32_t(CreepType::Count))) return false;
        if (!ArchiveVector(aArchive, Position, 0.0f, 500.0f)) return false;
        if (!ArchiveValue(aArchive, Health, 0.0f, 1000.0f)) return false;
        if (!ArchiveValue(aArchive, Damage, 0.0f, 10.0f)) return false;
        if (!ArchivePlayerID(aArchive, OwnerID)) return false;
        return ColliderParams.Archive(aArchive);
    }
};

inline bool operator==(const CreepInitData& aLHS, const CreepInitData& aRHS)
{
    return aLHS.Type == aRHS.Type && aLHS.Position == aRHS.Position && aLHS.OwnerID == aRHS.OwnerID;
}

using EntityInitData =
    std::variant<std::monostate, ProjectileInitData, TowerInitData, CreepInitData>;

template <>
struct fmt::formatter<EntityInitData> : fmt::formatter<std::string> {
    auto format(EntityInitData const& aObj, format_context& aCtx) const -> decltype(aCtx.out())
    {
        struct Visitor {
            format_context* Ctx;

            void operator()(const ProjectileInitData&) const
            {
                fmt::format_to(Ctx->out(), "projectile init data");
            }
            void operator()(const TowerInitData&) const
            {
                fmt::format_to(Ctx->out(), "tower init data");
            }
            void operator()(const CreepInitData&) const
            {
                fmt::format_to(Ctx->out(), "creep init data");
            }
            void operator()(const std::monostate&) const
            {
                fmt::format_to(Ctx->out(), "no init data");
            }
        };

        std::visit(Visitor{&aCtx}, aObj);
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
        return ArchiveValue(aArchive, Health, -10.0f, 1000.0f);
    }
};

inline bool operator==(const HealthUpdateResponse& aLHS, const HealthUpdateResponse& aRHS)
{
    return aLHS.Entity == aRHS.Entity && aLHS.Health == aRHS.Health;
}

struct GoldUpdateResponse {
    ::PlayerID Player;
    int        Balance;

    bool Archive(auto& aArchive)
    {
        if (!ArchivePlayerID(aArchive, Player)) return false;
        return ArchiveValue(aArchive, Balance, -100000, 100000);
    }

    auto operator<=>(const GoldUpdateResponse&) const = default;
};

struct CommonIncomeUpdateResponse {
    int Value;

    bool Archive(auto& aArchive) { return ArchiveValue(aArchive, Value, -1000000, 1000000); }

    auto operator<=>(const CommonIncomeUpdateResponse&) const = default;
};

struct PlayerEliminatedResponse {
    ::PlayerID              PlayerID;
    std::vector<::PlayerID> Ranking;

    bool Archive(auto& aArchive)
    {
        if (!ArchivePlayerID(aArchive, PlayerID)) return false;
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

struct AuthRequest {
    std::string        Token;
    bool               HasAESNI;
    CryptoKeys::Public PublicKey;

    bool Archive(auto& aArchive)
    {
        if (!ArchiveString(aArchive, Token, 1024)) return false;
        if (!ArchiveBool(aArchive, HasAESNI)) return false;
        return ArchiveArray(aArchive, PublicKey, uint8_t(0), std::numeric_limits<uint8_t>::max());
    }

    auto operator<=>(const AuthRequest&) const = default;
};

struct AuthResponse {
    PlayerID ID;
    bool     HasAESNI;
    bool     Success;

    bool Archive(auto& aArchive)
    {
        if (!ArchivePlayerID(aArchive, ID)) return false;
        if (!ArchiveBool(aArchive, HasAESNI)) return false;
        return ArchiveBool(aArchive, Success);
    }

    auto operator<=>(const AuthResponse&) const = default;
};

enum class PacketType : std::uint16_t {
    ClientSync,
    ServerSync,
    Ack,
    Nack,
    NewGame,
    Connected,
    Auth,
    Count,
};

using NetworkRequestPayload  = std::variant<std::monostate, SyncPayload, AuthRequest>;
using NetworkResponsePayload = std::variant<
    std::monostate,
    NewGameResponse,
    ConnectedResponse,
    ErrorResponse,
    SyncPayload,
    RigidBodyUpdateResponse,
    HealthUpdateResponse,
    GoldUpdateResponse,
    CommonIncomeUpdateResponse,
    PlayerEliminatedResponse,
    GameEndResponse,
    AuthResponse>;

template <typename _Payload>
struct NetworkEvent {
    PacketType Type;
    ::PlayerID PlayerID;
    uint32_t   Tick;
    _Payload   Payload;

    bool Archive(auto& aArchive)
    {
        if (!ArchiveValue(aArchive, Type, 0u, uint32_t(PacketType::Count))) return false;
        if (!ArchivePlayerID(aArchive, PlayerID)) return false;
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
        struct Visitor {
            format_context* Ctx;

            void operator()(const NewGameResponse& aResp) const
            {
                fmt::format_to(
                    Ctx->out(),
                    "new game response, game ID {}, your player {}, {} players",
                    aResp.GameID,
                    aResp.YourPlayerID,
                    aResp.Players.size());
            }
            void operator()(const ConnectedResponse&) const
            {
                fmt::format_to(Ctx->out(), "connected response");
            }
            void operator()(const ErrorResponse& aResp) const
            {
                fmt::format_to(Ctx->out(), "error response: {}", uint8_t(aResp.Error));
            }
            void operator()(const SyncPayload& aResp) const
            {
                fmt::format_to(Ctx->out(), "sync payload for game {}", aResp.GameID);
            }
            void operator()(const RigidBodyUpdateResponse& aResp) const
            {
                fmt::format_to(
                    Ctx->out(),
                    "rigid body update, {}, with {}",
                    aResp.Event,
                    aResp.InitData);
            }
            void operator()(const HealthUpdateResponse& aResp) const
            {
                fmt::format_to(Ctx->out(), "health update for {}: {}", aResp.Entity, aResp.Health);
            }
            void operator()(const GoldUpdateResponse& aResp) const
            {
                fmt::format_to(
                    Ctx->out(),
                    "gold update for player {}: {}",
                    aResp.Player,
                    aResp.Balance);
            }
            void operator()(const CommonIncomeUpdateResponse& aResp) const
            {
                fmt::format_to(Ctx->out(), "common income update with {}", aResp.Value);
            }
            void operator()(const PlayerEliminatedResponse& aResp) const
            {
                fmt::format_to(
                    Ctx->out(),
                    "player {} eliminated, current rankings: {}",
                    aResp.PlayerID,
                    aResp.Ranking);
            }
            void operator()(const GameEndResponse& aResp) const
            {
                fmt::format_to(Ctx->out(), "game ended, final rankings: {}", aResp.Ranking);
            }
            void operator()(const AuthResponse& aResp) const
            {
                fmt::format_to(
                    Ctx->out(),
                    "auth response, player {}, success: {}",
                    aResp.ID,
                    aResp.Success);
            }
            void operator()(const std::monostate&) const
            {
                fmt::format_to(Ctx->out(), "no network response payload");
            }
        };

        std::visit(Visitor{&aCtx}, aObj);
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
