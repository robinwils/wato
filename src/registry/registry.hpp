#pragma once

#include "core/types.hpp"
#include "entt/entity/registry.hpp"

using Registry = entt::basic_registry<entt::entity>;

using EntitySyncMap = std::unordered_map<entt::entity, entt::entity>;

using Observers = std::vector<entt::hashed_string>;

using namespace entt::literals;

bool IsPlayerEliminated(const Registry& aRegistry, PlayerID aID);

entt::entity FindPlayerEntity(const Registry& aRegistry, PlayerID aID);

std::vector<PlayerID> GetPlayerIDs(const Registry& aReg);

entt::entity GetTargetSpawnFor(Registry& aRegistry, PlayerID aID);

entt::entity GetSenderFor(Registry& aRegistry, PlayerID aID);
