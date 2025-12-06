#pragma once

#include <bx/bx.h>
#include <spdlog/spdlog.h>

#include <entt/core/hashed_string.hpp>
#include <entt/entity/entity.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <list>
#include <stdexcept>
#include <unordered_map>
#include <variant>
#include <vector>

#include "components/creep.hpp"
#include "components/player.hpp"
#include "components/tower.hpp"
#include "core/queue/ring_buffer.hpp"
#include "core/serialize.hpp"
#include "core/types.hpp"
#include "input/input.hpp"

using namespace entt::literals;

enum class ActionType {
    Move,
    SendCreep,
    BuildTower,
    EnterPlacementMode,
    ExitPlacementMode,
    Count,
};

template <>
struct fmt::formatter<ActionType> : fmt::formatter<std::string> {
    auto format(ActionType aObj, format_context& aCtx) const -> decltype(aCtx.out())
    {
        switch (aObj) {
            case ActionType::Move:
                return fmt::format_to(aCtx.out(), "Move");
            case ActionType::SendCreep:
                return fmt::format_to(aCtx.out(), "Send Creep");
            case ActionType::BuildTower:
                return fmt::format_to(aCtx.out(), "Build Tower");
            case ActionType::EnterPlacementMode:
                return fmt::format_to(aCtx.out(), "Enter Placement Mode");
            case ActionType::ExitPlacementMode:
                return fmt::format_to(aCtx.out(), "Exit Placement Mode");
            default:
                return fmt::format_to(aCtx.out(), "Unknown");
        }
    }
};

enum class ActionTag {
    FixedTime,
    FrameTime,
    Count,
};

template <>
struct fmt::formatter<ActionTag> : fmt::formatter<std::string> {
    auto format(ActionTag aObj, format_context& aCtx) const -> decltype(aCtx.out())
    {
        switch (aObj) {
            case ActionTag::FixedTime:
                return fmt::format_to(aCtx.out(), "Fixed Time");
            case ActionTag::FrameTime:
                return fmt::format_to(aCtx.out(), "Frame Time");
            default:
                return fmt::format_to(aCtx.out(), "Unknown");
        }
    }
};

struct KeyState {
    enum class State { PressOnce, Hold };
    using InputButton = std::variant<Keyboard::Key, Mouse::Button>;

    KeyState() = delete;
    KeyState(InputButton aKey, State aState, uint8_t aModifiers)
        : Key(aKey), State(aState), Modifiers(aModifiers)
    {
    }
    InputButton Key;
    State       State;
    uint8_t     Modifiers;
};

enum class MoveDirection { Left, Right, Front, Back, Up, Down, Count };

struct MovePayload {
    MoveDirection Direction;

    bool Archive(auto& aArchive)
    {
        if (!ArchiveValue(aArchive, Direction, 0u, uint32_t(MoveDirection::Count))) return false;
        return true;
    }
};

inline bool operator==(const MovePayload& aLHS, const MovePayload& aRHS)
{
    return aLHS.Direction == aRHS.Direction;
}

struct SendCreepPayload {
    CreepType Type;

    bool Archive(auto& aArchive)
    {
        if (!ArchiveValue(aArchive, Type, 0u, uint32_t(CreepType::Count))) return false;
        return true;
    }
};

inline bool operator==(const SendCreepPayload& aLHS, const SendCreepPayload& aRHS)
{
    return aLHS.Type == aRHS.Type;
}

struct BuildTowerPayload {
    TowerType    Tower;
    glm::vec3    Position{0.0f};
    entt::entity CliPredictedEntity{entt::null};

    bool Archive(auto& aArchive)
    {
        if (!ArchiveValue(aArchive, Tower, 0u, uint32_t(TowerType::Count))) return false;
        if (!ArchiveVector(aArchive, Position, 0.0f, 20.0f)) return false;
        if (!ArchiveValue(aArchive, CliPredictedEntity, 0, 1000000)) return false;
        return true;
    }
};

inline bool operator==(const BuildTowerPayload& aLHS, const BuildTowerPayload& aRHS)
{
    return aLHS.Tower == aRHS.Tower && aLHS.Position == aRHS.Position
           && aLHS.CliPredictedEntity == aRHS.CliPredictedEntity;
}

struct PlacementModePayload {
    bool      CanBuild;
    TowerType Tower;

    bool Archive(auto& aArchive)
    {
        if (!ArchiveBool(aArchive, CanBuild)) return false;
        if (!ArchiveValue(aArchive, Tower, 0u, uint32_t(TowerType::Count))) return false;
        return true;
    }
};

inline bool operator==(const PlacementModePayload& aLHS, const PlacementModePayload& aRHS)
{
    return aLHS.CanBuild == aRHS.CanBuild && aLHS.Tower == aRHS.Tower;
}

struct Action {
    using payload_type =
        std::variant<MovePayload, SendCreepPayload, BuildTowerPayload, PlacementModePayload>;
    ActionType   Type;
    ActionTag    Tag;
    payload_type Payload;
    bool         IsProcessed = false;

    void AddExtraInputInfo(const Input& aInput)
    {
        std::visit(
            VariantVisitor{
                [&](MovePayload&) {},
                [&](SendCreepPayload&) {},
                [&](BuildTowerPayload& aPayload) {
                    BX_ASSERT(
                        aInput.MouseWorldIntersect().has_value(),
                        "input has no mouse intersection");
                    aPayload.Position = *aInput.MouseWorldIntersect();
                },
                [&](PlacementModePayload&) {},
            },
            Payload);
    }

    bool Archive(auto& aArchive)
    {
        if (!ArchiveValue(aArchive, Type, 0u, uint32_t(ActionType::Count))) return false;
        if (!ArchiveValue(aArchive, Tag, 0u, uint32_t(ActionTag::Count))) return false;
        if (!ArchiveVariant(aArchive, Payload)) return false;
        return true;
    }
};

inline bool operator==(const Action& aLHS, const Action& aRHS)
{
    return aLHS.Type == aRHS.Type && aLHS.Tag == aRHS.Tag && aLHS.Payload == aRHS.Payload
           && aLHS.IsProcessed == aRHS.IsProcessed;
}

using ActionsType = std::vector<Action>;

template <>
struct fmt::formatter<Action::payload_type> : fmt::formatter<std::string> {
    auto format(Action::payload_type aObj, format_context& aCtx) const -> decltype(aCtx.out())
    {
        return std::visit(
            [&](const auto& aPayload) -> decltype(aCtx.out()) {
                using T = std::decay_t<decltype(aPayload)>;
                if constexpr (std::is_same_v<T, MovePayload>) {
                    switch (aPayload.Direction) {
                        case MoveDirection::Left:
                            return fmt::format_to(aCtx.out(), "Left");
                        case MoveDirection::Right:
                            return fmt::format_to(aCtx.out(), "Right");
                        case MoveDirection::Front:
                            return fmt::format_to(aCtx.out(), "Front");
                        case MoveDirection::Back:
                            return fmt::format_to(aCtx.out(), "Back");
                        case MoveDirection::Up:
                            return fmt::format_to(aCtx.out(), "Up");
                        case MoveDirection::Down:
                            return fmt::format_to(aCtx.out(), "Down");
                        case MoveDirection::Count:
                            return fmt::format_to(aCtx.out(), "Count");
                    }
                } else if constexpr (std::is_same_v<T, SendCreepPayload>) {
                    return fmt::format_to(
                        aCtx.out(),
                        "CreepType: {}",
                        CreepTypeToString(aPayload.Type));
                } else if constexpr (std::is_same_v<T, BuildTowerPayload>) {
                    return fmt::format_to(
                        aCtx.out(),
                        "TowerType: {}",
                        TowerTypeToString(aPayload.Tower));
                } else if constexpr (std::is_same_v<T, PlacementModePayload>) {
                    return fmt::format_to(
                        aCtx.out(),
                        "{} Tower",
                        TowerTypeToString(aPayload.Tower));
                }
                return fmt::format_to(aCtx.out(), "Unknown");
            },
            aObj);
    }
};

template <>
struct fmt::formatter<Action> : fmt::formatter<std::string> {
    auto format(Action aObj, format_context& aCtx) const -> decltype(aCtx.out())
    {
        return fmt::format_to(aCtx.out(), "{} {} Action <{}>", aObj.Type, aObj.Tag, aObj.Payload);
    }
};

constexpr Action kMoveLeftAction = Action{
    .Type    = ActionType::Move,
    .Tag     = ActionTag::FrameTime,
    .Payload = MovePayload{.Direction = MoveDirection::Left},
};

constexpr Action kMoveRightAction = Action{
    .Type    = ActionType::Move,
    .Tag     = ActionTag::FrameTime,
    .Payload = MovePayload{.Direction = MoveDirection::Right},
};

constexpr Action kMoveFrontAction = Action{
    .Type    = ActionType::Move,
    .Tag     = ActionTag::FrameTime,
    .Payload = MovePayload{.Direction = MoveDirection::Front},
};

constexpr Action kMoveBackAction = Action{
    .Type    = ActionType::Move,
    .Tag     = ActionTag::FrameTime,
    .Payload = MovePayload{.Direction = MoveDirection::Back},
};

constexpr Action kMoveUpAction = Action{
    .Type    = ActionType::Move,
    .Tag     = ActionTag::FrameTime,
    .Payload = MovePayload{.Direction = MoveDirection::Up},
};

constexpr Action kMoveDownAction = Action{
    .Type    = ActionType::Move,
    .Tag     = ActionTag::FrameTime,
    .Payload = MovePayload{.Direction = MoveDirection::Down},
};

constexpr Action kEnterPlacementModeAction = Action{
    .Type    = ActionType::EnterPlacementMode,
    .Tag     = ActionTag::FrameTime,
    .Payload = PlacementModePayload{.CanBuild = true, .Tower = TowerType::Arrow},
};

constexpr Action kBuildTowerAction = Action{
    .Type    = ActionType::BuildTower,
    .Tag     = ActionTag::FixedTime,
    .Payload = BuildTowerPayload{.Tower = TowerType::Arrow},
};

constexpr Action kExitPlacementModeAction = Action{
    .Type    = ActionType::ExitPlacementMode,
    .Tag     = ActionTag::FrameTime,
    .Payload = PlacementModePayload{},
};

constexpr Action kSendCreepAction = Action{
    .Type    = ActionType::SendCreep,
    .Tag     = ActionTag::FixedTime,
    .Payload = SendCreepPayload{.Type = CreepType::Simple},
};

struct ActionBinding {
    ActionBinding() = delete;
    ActionBinding(struct KeyState aKeyState, struct Action aAction)
        : KeyState(aKeyState), Action(aAction)
    {
    }
    struct KeyState KeyState;
    struct Action   Action;
};

class ActionBindings
{
   public:
    using bindings_type = std::unordered_map<std::string, ActionBinding>;

    void AddBinding(const std::string& aActionStr, const KeyState& aState, const Action& aAction);
    ActionsType           ActionsFromInput(const Input& aInput);
    static ActionBindings Defaults();
    static ActionBindings PlacementDefaults();

   private:
    bindings_type mBindings;
};

struct NormalPayload {
};

struct ActionContext {
    using payload_type = std::variant<NormalPayload, PlacementModePayload>;
    enum class State {
        Default,  // rename ?
        Placement,
        Server,
    };
    State          State;
    ActionBindings Bindings;
    payload_type   Payload;
};

using ActionContextStack = std::list<ActionContext>;

#ifndef DOCTEST_CONFIG_DISABLE
#include "test.hpp"
TEST_CASE("encode.action")
{
    StreamEncoder enc;
    Action        ae1 = kBuildTowerAction;
    Action        ae2 = kSendCreepAction;

    ae1.Archive(enc);
    ae2.Archive(enc);

    StreamDecoder dec(enc.Data());
    Action        ad1;
    Action        ad2;

    CHECK_EQ(ad1.Archive(dec), true);
    CHECK_EQ(ad1, ae1);

    CHECK_EQ(ad2.Archive(dec), true);
    CHECK_EQ(ad2, ae2);
}
#endif
