#include "core/snapshot.hpp"

#include <entt/entity/fwd.hpp>
#include <entt/entity/snapshot.hpp>

#include "fmt/base.h"

template <typename OutArchive>
void SaveRegistry(const entt::registry& aRegistry, OutArchive& aArchive)
{
    // TODO: Header with version
    entt::snapshot{aRegistry}
        .template get<entt::entity>(aArchive)
        .template get<Transform3D>(aArchive)
        .template get<Health>(aArchive);
    // .template get<Physics>(aArchive);
    //
    const auto& phy = aRegistry.ctx().get<const Physics&>();
    Physics::Serialize(aArchive, phy);
}

template <typename InArchive>
void LoadRegistry(entt::registry& aRegistry, InArchive& aArchive)
{
    // TODO: Header with version
    entt::snapshot_loader{aRegistry}
        .template get<entt::entity>(aArchive)
        .template get<Transform3D>(aArchive)
        .template get<Health>(aArchive);
    Physics::Deserialize(aArchive, aRegistry);
    // .template get<Physics>(aArchive);
    // .template get<Physics>([](auto& reg, entt::entity e, auto& ar) {});
}
