#pragma once

#include <spdlog/spdlog.h>

#include <entt/core/type_info.hpp>

#include "core/sys/log.hpp"
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

template <typename Type>
[[nodiscard]] const Type& GetSingletonComponent(
    const Registry&     aRegistry,
    const entt::id_type aId = entt::type_id<Type>().hash())
{
#if WATO_DEBUG
    if (!aRegistry.ctx().contains<Type>(aId)) {
        SPDLOG_LOGGER_CRITICAL(
            WATO_REG_LOGGER(aRegistry),
            "singleton component {} not initialized",
            entt::type_id<Type>().name());
    }
#endif
    return aRegistry.ctx().get<Type>(aId);
}

template <typename Type>
[[nodiscard]] Type& GetSingletonComponent(
    Registry&           aRegistry,
    const entt::id_type aId = entt::type_id<Type>().hash())
{
#if WATO_DEBUG
    if (!aRegistry.ctx().contains<Type>(aId)) {
        SPDLOG_LOGGER_CRITICAL(
            WATO_REG_LOGGER(aRegistry),
            "singleton component {} not initialized",
            entt::type_id<Type>().name());
    }
#endif
    return aRegistry.ctx().get<Type>(aId);
}
