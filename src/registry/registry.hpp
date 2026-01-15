#pragma once

#include "components/player.hpp"
#include "entt/entity/registry.hpp"

using Registry = entt::basic_registry<entt::entity>;

using EntitySyncMap = std::unordered_map<entt::entity, entt::entity>;

using Observers = std::vector<entt::hashed_string>;

using namespace entt::literals;

bool IsPlayerEliminated(const Registry& aRegistry, PlayerID aID);
