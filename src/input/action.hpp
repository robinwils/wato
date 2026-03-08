#pragma once

#include <bx/bx.h>
#include <spdlog/spdlog.h>

#include <entt/core/hashed_string.hpp>
#include <entt/entity/entity.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <optional>
#include <unordered_map>
#include <variant>
#include <vector>

#include "components/creep.hpp"
#include "components/player.hpp"
#include "components/tower.hpp"
#include "core/serialize.hpp"
#include "core/types.hpp"
#include "input/input.hpp"
#include "registry/registry.hpp"

using namespace entt::literals;

struct KeyState {
    enum class State { PressOnce, Hold };
    using InputButton = std::variant<Keyboard::Key, Mouse::Button>;

    KeyState() = delete;
    KeyState(InputButton aKey, State aState, uint8_t aModifiers)
        : Key(aKey), State(aState), Modifiers(aModifiers)
    {
    }
    bool IsTriggered(const Input& aInput) const;

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

    auto operator<=>(const MovePayload&) const = default;
};

struct SendCreepPayload {
    CreepType Type;

    bool Archive(auto& aArchive)
    {
        if (!ArchiveValue(aArchive, Type, 0u, uint32_t(CreepType::Count))) return false;
        return true;
    }

    auto operator<=>(const SendCreepPayload&) const = default;
};

struct BuildTowerPayload {
    TowerType Tower;
    glm::vec3 Position{0.0f};

    bool Archive(auto& aArchive)
    {
        if (!ArchiveValue(aArchive, Tower, 0u, uint32_t(TowerType::Count))) return false;
        if (!ArchiveVector(aArchive, Position, 0.0f, 100.0f)) return false;
        return true;
    }
};

inline bool operator==(const BuildTowerPayload& aLHS, const BuildTowerPayload& aRHS)
{
    return aLHS.Tower == aRHS.Tower && aLHS.Position == aRHS.Position;
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

    auto operator<=>(const PlacementModePayload&) const = default;
};

struct Action {
    using payload_type =
        std::variant<MovePayload, SendCreepPayload, BuildTowerPayload, PlacementModePayload>;
    payload_type Payload;

    void AddExtraInputInfo(const Input& aInput)
    {
        struct Visitor {
            const Input* In;

            void operator()(MovePayload&) const {}
            void operator()(SendCreepPayload&) const {}
            void operator()(BuildTowerPayload& aPayload) const
            {
                if (In->MouseWorldIntersect().has_value()) {
                    aPayload.Position = *In->MouseWorldIntersect();
                }
            }
            void operator()(PlacementModePayload&) const {}
        };

        std::visit(Visitor{&aInput}, Payload);
    }

    bool Archive(auto& aArchive) { return ArchiveVariant(aArchive, Payload); }

    bool IsFrameTime() const
    {
        return std::holds_alternative<MovePayload>(Payload)
               || std::holds_alternative<PlacementModePayload>(Payload);
    }
};

inline bool operator==(const Action& aLHS, const Action& aRHS)
{
    return aLHS.Payload == aRHS.Payload;
}

using ActionsType = std::vector<Action>;

struct TaggedAction {
    ::PlayerID PlayerID;
    ::Action   Action;
};

using TaggedActionsType = std::vector<TaggedAction>;

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
        return fmt::format_to(aCtx.out(), "Action <{}>", aObj.Payload);
    }
};

constexpr Action kMoveLeftAction = Action{
    .Payload = MovePayload{.Direction = MoveDirection::Left},
};

constexpr Action kMoveRightAction = Action{
    .Payload = MovePayload{.Direction = MoveDirection::Right},
};

constexpr Action kMoveFrontAction = Action{
    .Payload = MovePayload{.Direction = MoveDirection::Front},
};

constexpr Action kMoveBackAction = Action{
    .Payload = MovePayload{.Direction = MoveDirection::Back},
};

constexpr Action kMoveUpAction = Action{
    .Payload = MovePayload{.Direction = MoveDirection::Up},
};

constexpr Action kMoveDownAction = Action{
    .Payload = MovePayload{.Direction = MoveDirection::Down},
};

constexpr Action kEnterPlacementModeAction = Action{
    .Payload = PlacementModePayload{.CanBuild = true, .Tower = TowerType::Arrow},
};

constexpr Action kBuildTowerAction = Action{
    .Payload = BuildTowerPayload{.Tower = TowerType::Arrow},
};

constexpr Action kExitPlacementModeAction = Action{
    .Payload = PlacementModePayload{},
};

constexpr Action kSendCreepAction = Action{
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
    static ActionBindings Defaults();
    static ActionBindings PlacementDefaults();

    template <typename V>
    void Visit(V&& aVisitor)
    {
        for (auto& [_, binding] : mBindings) {
            aVisitor(binding);
        }
    }

   private:
    bindings_type mBindings;
};

using FrameActionBuffer = ActionsType;

struct PlacementState {
    TowerType Tower;
};

struct ContextEntry {
    ActionBindings                               Bindings;
    std::variant<std::monostate, PlacementState> Data;
};

struct ActionContextStack {
    std::vector<ContextEntry> Stack = {{ActionBindings::Defaults(), std::monostate{}}};

    ActionBindings& CurrentBindings() { return Stack.back().Bindings; }

    template <typename T>
    T* GetState()
    {
        return std::get_if<T>(&Stack.back().Data);
    }

    void EnterPlacement(Registry& aRegistry, TowerType aTower);
    void ExitPlacement(Registry& aRegistry);
    void TogglePlacement(Registry& aRegistry, const PlacementModePayload& aPayload);
};

