#pragma once

#include "entt/entity/registry.hpp"

using Registry = entt::basic_registry<entt::entity>;

using EntitySyncMap = std::unordered_map<entt::entity, entt::entity>;
