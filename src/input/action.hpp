#pragma once

#include <entt/core/hashed_string.hpp>
#include <glm/ext/vector_float3.hpp>
#include <map>
#include <variant>
#include <vector>

#include "core/queue/ring_buffer.hpp"
#include "input/input.hpp"

template <typename ActionType>
struct ActionBase {
   private:
    using container_type = std::map<Keyboard::Key, Button::State>;
    ActionBase(const container_type& aBinding) : mBinding(aBinding) {}

    container_type mBinding;

    friend ActionType;
};

struct MoveAction : public ActionBase<MoveAction> {
    MoveAction() : ActionBase<MoveAction>({}) {}

    glm::vec3 Direction;
};

struct SendCreepAction : public ActionBase<SendCreepAction> {
    entt::hashed_string Type;
};

using Action       = std::variant<MoveAction, SendCreepAction>;
using ActionBuffer = RingBuffer<Action, 128>;

template <class... Ts>
struct ActionVisitor : Ts... {
    using Ts::operator()...;
};
