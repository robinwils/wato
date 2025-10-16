#pragma once

#include "entt/entity/registry.hpp"

using Registry = entt::basic_registry<entt::entity>;

template <typename Cmp>
struct ComponentDiff {
    Cmp State;
    Cmp Registry;
};

template <typename Cmp>
entt::storage<ComponentDiff<Cmp>> DiffCmpWithNamedPool(
    Registry&           aRegistry,
    const entt::id_type aStorageID)
{
    auto  regView = aRegistry.view<Cmp>();
    auto& aa      = aRegistry.storage<Cmp>(aStorageID);

    for (auto entity : regView) {
    }
}
