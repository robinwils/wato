#pragma once

#include <entt/core/hashed_string.hpp>
#include <list>
#include <unordered_map>
#include <variant>
#include <vector>

#include "core/queue/ring_buffer.hpp"
#include "input/input.hpp"

enum class ActionType {
    Move,
    SendCreep,
    BuildTower,
    PopCtx,
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
    enum class Direction { Left, Right, Front, Back };
    Direction Direction;
};

struct SendCreepPayload {
    entt::hashed_string Type;
};

struct BuildTowerPayload {
    entt::hashed_string Tower;
};

struct Action {
    using payload_type = std::variant<MovePayload, SendCreepPayload, BuildTowerPayload>;
    ActionType   Type;
    ActionTag    Tag;
    payload_type Payload;
};

constexpr Action kMoveLeftAction = Action{.Type = ActionType::Move,
    .Tag                                        = ActionTag::FrameTime,
    .Payload = MovePayload{.Direction = MovePayload::Direction::Left}};

constexpr Action kMoveRightAction = Action{.Type = ActionType::Move,
    .Tag                                         = ActionTag::FrameTime,
    .Payload = MovePayload{.Direction = MovePayload::Direction::Right}};

constexpr Action kMoveFrontAction = Action{.Type = ActionType::Move,
    .Tag                                         = ActionTag::FrameTime,
    .Payload = MovePayload{.Direction = MovePayload::Direction::Front}};

constexpr Action kMoveBackAction = Action{.Type = ActionType::Move,
    .Tag                                        = ActionTag::FrameTime,
    .Payload = MovePayload{.Direction = MovePayload::Direction::Back}};

constexpr Action kBuildTowerAction = Action{.Type = ActionType::Move,
    .Tag                                          = ActionTag::FixedTime,
    .Payload                                      = BuildTowerPayload{.Tower = ""}};

constexpr Action kPopCtxAction = Action{.Type = ActionType::PopCtx,
    .Tag                                      = ActionTag::FrameTime,
    .Payload = MovePayload{.Direction = MovePayload::Direction::Back}};

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

struct PlacementPayload {
    entt::hashed_string Tower;
};

struct ActionContext {
    using payload_type = std::variant<NormalPayload, PlacementPayload>;
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

template <class... Ts>
struct InputButtonVisitor : Ts... {
    using Ts::operator()...;
};
