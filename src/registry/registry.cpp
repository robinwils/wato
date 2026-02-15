#include "registry/registry.hpp"

#include <entt/entity/entity.hpp>

#include "components/player.hpp"

bool IsPlayerEliminated(const Registry& aRegistry, PlayerID aID)
{
    for (const auto&& [entity, player] : aRegistry.view<Player, Eliminated>().each()) {
        if (player.ID == aID) {
            return true;
        }
    }
    return false;
}

entt::entity FindPlayerEntity(const Registry& aRegistry, PlayerID aID)
{
    for (const auto&& [entity, player] : aRegistry.view<Player>().each()) {
        if (player.ID == aID) {
            return entity;
        }
    }
    return entt::null;
}

std::vector<PlayerID> GetPlayerIDs(const Registry& aReg)
{
    std::vector<PlayerID> pIDS;

    for (const auto&& [e, p] : aReg.view<Player>()->each()) {
        pIDS.push_back(p.ID);
    }

    return pIDS;
}
