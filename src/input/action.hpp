#pragma once

#include <spdlog/spdlog.h>

#include <entt/core/hashed_string.hpp>
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
#include "core/types.hpp"
#include "input/input.hpp"

using namespace entt::literals;

enum class ActionType {
    Move,
    SendCreep,
    BuildTower,
    EnterPlacementMode,
    ExitPlacementMode,
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
        }
    }
};

enum class ActionTag {
    FixedTime,
    FrameTime,
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
        }
    }
};

struct KeyState {
    using InputButton = std::variant<Keyboard::Key, Mouse::Button>;
    enum class State { PressOnce, Hold };
    InputButton Key;
    State       State;
    uint8_t     Modifiers;
};

enum class MoveDirection { Left, Right, Front, Back, Up, Down };

struct MovePayload {
    MoveDirection Direction;
};

struct SendCreepPayload {
    CreepType Type;
};

struct BuildTowerPayload {
    TowerType Tower;
    glm::vec3 Position;
};

struct PlacementModePayload {
    bool      CanBuild;
    TowerType Tower;
};

struct Action {
    using payload_type =
        std::variant<MovePayload, SendCreepPayload, BuildTowerPayload, PlacementModePayload>;
    ActionType   Type;
    ActionTag    Tag;
    payload_type Payload;

    constexpr static auto Serialize(auto& aArchive, const auto& aSelf)
    {
        aArchive.template Write<ActionType>(&aSelf.Type, 1);
        aArchive.template Write<ActionTag>(&aSelf.Tag, 1);

        // Then serialize the actual payload based on type
        std::visit(
            VariantVisitor{
                [&](const MovePayload& aPayload) {
                    aArchive.template Write<MoveDirection>(&aPayload.Direction, 1);
                },
                [&](const SendCreepPayload& aPayload) {
                    aArchive.template Write<CreepType>(&aPayload.Type, 1);
                },
                [&](const BuildTowerPayload& aPayload) {
                    aArchive.template Write<TowerType>(&aPayload.Tower, 1);
                    aArchive.template Write<float>(&aPayload.Position, 3);
                },
                [&](const PlacementModePayload& aPayload) {
                    aArchive.template Write<bool>(&aPayload.CanBuild, 1);
                    aArchive.template Write<TowerType>(&aPayload.Tower, 1);
                },
            },
            aSelf.Payload);
    }

    constexpr static auto Deserialize(auto& aArchive, auto& aSelf)
    {
        aArchive.template Read<ActionType>(&aSelf.Type, 1);
        aArchive.template Read<ActionTag>(&aSelf.Tag, 1);

        switch (aSelf.Type) {
            case ActionType::Move: {
                MovePayload payload{};
                aArchive.template Read<MoveDirection>(&payload.Direction, 1);
                aSelf.Payload = payload;
                break;
            }
            case ActionType::SendCreep: {
                SendCreepPayload payload{};
                aArchive.template Read<CreepType>(&payload.Type, 1);
                aSelf.Payload = payload;
                break;
            }
            case ActionType::BuildTower: {
                BuildTowerPayload payload{};
                aArchive.template Read<TowerType>(&payload.Tower, 1);
                aArchive.template Read<float>(glm::value_ptr(payload.Position), 3);
                aSelf.Payload = payload;
                break;
            }
            case ActionType::EnterPlacementMode:
            case ActionType::ExitPlacementMode: {
                PlacementModePayload payload{};
                aArchive.template Read<bool>(&payload.CanBuild, 1);
                aArchive.template Read<TowerType>(&payload.Tower, 1);
                aSelf.Payload = payload;
                break;
            }
            default:
                return false;
        }

        return true;
    }
};

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
    .Payload = MovePayload{.Direction = MoveDirection::Left}};

constexpr Action kMoveRightAction = Action{
    .Type    = ActionType::Move,
    .Tag     = ActionTag::FrameTime,
    .Payload = MovePayload{.Direction = MoveDirection::Right}};

constexpr Action kMoveFrontAction = Action{
    .Type    = ActionType::Move,
    .Tag     = ActionTag::FrameTime,
    .Payload = MovePayload{.Direction = MoveDirection::Front}};

constexpr Action kMoveBackAction = Action{
    .Type    = ActionType::Move,
    .Tag     = ActionTag::FrameTime,
    .Payload = MovePayload{.Direction = MoveDirection::Back}};

constexpr Action kMoveUpAction = Action{
    .Type    = ActionType::Move,
    .Tag     = ActionTag::FrameTime,
    .Payload = MovePayload{.Direction = MoveDirection::Up}};

constexpr Action kMoveDownAction = Action{
    .Type    = ActionType::Move,
    .Tag     = ActionTag::FrameTime,
    .Payload = MovePayload{.Direction = MoveDirection::Down}};

constexpr Action kEnterPlacementModeAction = Action{
    .Type    = ActionType::EnterPlacementMode,
    .Tag     = ActionTag::FrameTime,
    .Payload = PlacementModePayload{.CanBuild = true, .Tower = TowerType::Arrow}
};

constexpr Action kBuildTowerAction = Action{
    .Type    = ActionType::BuildTower,
    .Tag     = ActionTag::FixedTime,
    .Payload = BuildTowerPayload{.Tower = TowerType::Arrow}};

constexpr Action kExitPlacementModeAction = Action{
    .Type    = ActionType::ExitPlacementMode,
    .Tag     = ActionTag::FrameTime,
    .Payload = PlacementModePayload{}};

constexpr Action kSendCreepAction = Action{
    .Type    = ActionType::SendCreep,
    .Tag     = ActionTag::FixedTime,
    .Payload = SendCreepPayload{.Type = CreepType::Simple}};

struct ActionBinding {
    struct KeyState KeyState;
    struct Action   Action;
};

class ActionBindings
{
   public:
    using bindings_type = std::unordered_map<std::string, ActionBinding>;
    using actions_type  = std::vector<Action>;

    void AddBinding(const std::string& aActionStr, const KeyState& aState, const Action& aAction);
    actions_type          ActionsFromInput(const Input& aInput);
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

struct PlayerActions {
    using actions_type = std::vector<Action>;

    constexpr static auto Serialize(auto& aArchive, const auto& aSelf)
    {
        actions_type::size_type nActions = aSelf.Actions.size();

        aArchive.template Write<PlayerID>(&aSelf.Player, 1);
        aArchive.template Write<GameInstanceID>(&aSelf.GameID, 1);
        spdlog::info("game instance ID serialized = {}", aSelf.GameID);
        aArchive.template Write<uint32_t>(&aSelf.Tick, 1);
        aArchive.template Write<actions_type::size_type>(&nActions, 1);
        for (const Action& action : aSelf.Actions) {
            Action::Serialize(aArchive, action);
        }
    }
    constexpr static auto Deserialize(auto& aArchive, auto& aSelf)
    {
        actions_type::size_type nActions = 0;
        aArchive.template Read<PlayerID>(&aSelf.Player, 1);
        aArchive.template Read<GameInstanceID>(&aSelf.GameID, 1);
        spdlog::info("game instance ID deserialized = {}", aSelf.GameID);
        aArchive.template Read<uint32_t>(&aSelf.Tick, 1);
        aArchive.template Read<actions_type::size_type>(&nActions, 1);
        for (actions_type::size_type idx = 0; idx < nActions; idx++) {
            Action action;
            if (!Action::Deserialize(aArchive, action)) {
                throw std::runtime_error("cannot deserialize action");
            }
            aSelf.Actions.push_back(action);
        }
        return true;
    }
    PlayerID       Player;
    GameInstanceID GameID;
    uint32_t       Tick;
    actions_type   Actions;
};

using ActionContextStack = std::list<ActionContext>;
using ActionBuffer       = RingBuffer<PlayerActions, 128>;
