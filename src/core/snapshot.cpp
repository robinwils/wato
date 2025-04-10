#include "core/snapshot.hpp"

#include <entt/entity/fwd.hpp>
#include <entt/entity/snapshot.hpp>

#include "fmt/base.h"

template <typename OutArchive>
void SaveRegistry(const entt::registry& aRegistry, OutArchive& aArchive)
{
    entt::snapshot{aRegistry}
        .template get<entt::entity>(aArchive)
        .template get<Transform3D>(aArchive)
        .template get<Health>(aArchive)
        .template get<Physics>(aArchive);
}

template <typename InArchive>
void LoadRegistry(entt::registry& aRegistry, InArchive& aArchive)
{
    entt::snapshot_loader{aRegistry}
        .template get<entt::entity>(aArchive)
        .template get<Transform3D>(aArchive)
        .template get<Health>(aArchive)
        .template get<Physics>(aArchive);
    // .template get<Physics>([](auto& reg, entt::entity e, auto& ar) {});
}
