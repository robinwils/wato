#pragma once

#include <entt/core/hashed_string.hpp>
#include <list>
#include <unordered_map>
#include <variant>
#include <vector>

#include "components/creep.hpp"
#include "components/tower.hpp"
#include "core/queue/ring_buffer.hpp"
#include "core/snapshot.hpp"
#include "input/input.hpp"

using namespace entt::literals;

enum class ActionType {
    Move,
    SendCreep,
    BuildTower,
    EnterPlacementMode,
    ExitPlacementMode,
};

enum class ActionTag {
    FixedTime,
    FrameTime,
};

struct KeyState {
    using InputButton = std::variant<Keyboard::Key, Mouse::Button>;
    enum class State { PressOnce, Hold };
    InputButton Key;
    State       State;
    uint8_t     Modifiers;
};

struct MovePayload {
    enum class Direction { Left, Right, Front, Back, Up, Down };
    Direction Direction;
};

struct SendCreepPayload {
    CreepType Type;
};

struct BuildTowerPayload {
    TowerType Tower;
};

struct PlacementModePayload {
    bool      CanBuild;
    TowerType Tower;
};

template <class... Ts>
struct VariantVisitor : Ts... {
    using Ts::operator()...;
};

struct Action {
    using payload_type =
        std::variant<MovePayload, SendCreepPayload, BuildTowerPayload, PlacementModePayload>;
    ActionType   Type;
    ActionTag    Tag;
    payload_type Payload;

    std::string String() const;

    constexpr static auto Serialize(auto& aArchive, const auto& aSelf)
    {
        aArchive.template Write<int>(&aSelf.Type, 1);
        aArchive.template Write<int>(&aSelf.Tag, 1);

        // Write payload type index first
        size_t index = aSelf.Payload.index();
        aArchive.template Write<size_t>(&index, 1);

        // Then serialize the actual payload based on type
        std::visit(
            VariantVisitor{
                [&](const MovePayload& aPayload) {
                    aArchive.template Write<int>(&aPayload.Direction, 1);
                },
                [&](const SendCreepPayload& aPayload) {
                    aArchive.template Write<int>(&aPayload.Type, 1);
                },
                [&](const BuildTowerPayload& aPayload) {
                    aArchive.template Write<int>(&aPayload.Tower, 1);
                },
                [&](const PlacementModePayload& aPayload) {
                    aArchive.template Write<bool>(&aPayload.CanBuild, 1);
                    aArchive.template Write<int>(&aPayload.Tower, 1);
                },
            },
            aSelf.Payload);
    }

    constexpr static auto Deserialize(auto& aArchive, auto& aSelf)
    {
        aArchive.template Read<int>(&aSelf.Type, 1);
        aArchive.template Read<int>(&aSelf.Tag, 1);

        // Read payload type index
        size_t index;
        aArchive.template Read<int>(&index, 1);

        // Deserialize the correct payload type based on index
        switch (index) {
            case 0: {  // MovePayload
                MovePayload payload{};
                aArchive.template Read<int>(&payload.Direction, 1);
                aSelf.Payload = payload;
                break;
            }
            case 1: {  // SendCreepPayload
                SendCreepPayload payload{};
                aArchive.template Read<int>(&payload.Type, 1);
                aSelf.Payload = payload;
                break;
            }
            case 2: {  // BuildTowerPayload
                BuildTowerPayload payload{};
                aArchive.template Read<int>(&payload.Tower, 1);
                aSelf.Payload = payload;
                break;
            }
            case 3: {  // PlacementModePayload
                PlacementModePayload payload{};
                aArchive.template Read<bool>(&payload.CanBuild, 1);
                aArchive.template Read<int>(&payload.Tower, 1);
                aSelf.Payload = payload;
                break;
            }
            default:
                return false;
        }

        return true;
    }
};

constexpr Action kMoveLeftAction = Action{
    .Type    = ActionType::Move,
    .Tag     = ActionTag::FrameTime,
    .Payload = MovePayload{.Direction = MovePayload::Direction::Left}};

constexpr Action kMoveRightAction = Action{
    .Type    = ActionType::Move,
    .Tag     = ActionTag::FrameTime,
    .Payload = MovePayload{.Direction = MovePayload::Direction::Right}};

constexpr Action kMoveFrontAction = Action{
    .Type    = ActionType::Move,
    .Tag     = ActionTag::FrameTime,
    .Payload = MovePayload{.Direction = MovePayload::Direction::Front}};

constexpr Action kMoveBackAction = Action{
    .Type    = ActionType::Move,
    .Tag     = ActionTag::FrameTime,
    .Payload = MovePayload{.Direction = MovePayload::Direction::Back}};

constexpr Action kMoveUpAction = Action{
    .Type    = ActionType::Move,
    .Tag     = ActionTag::FrameTime,
    .Payload = MovePayload{.Direction = MovePayload::Direction::Up}};

constexpr Action kMoveDownAction = Action{
    .Type    = ActionType::Move,
    .Tag     = ActionTag::FrameTime,
    .Payload = MovePayload{.Direction = MovePayload::Direction::Down}};

constexpr Action kEnterPlacementModeAction = Action{
    .Type    = ActionType::EnterPlacementMode,
    .Tag     = ActionTag::FixedTime,
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
    };
    State          State;
    ActionBindings Bindings;
    payload_type   Payload;
};

struct PlayerActions {
    using actions_type = std::vector<Action>;

    uint32_t     Tick;
    actions_type Actions;
};

using ActionContextStack = std::list<ActionContext>;
using ActionBuffer       = RingBuffer<PlayerActions, 128>;
