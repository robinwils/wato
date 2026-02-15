#include "registry/registry.hpp"

#include <entt/entity/entity.hpp>
#include <map>

#include "components/health.hpp"
#include "components/player.hpp"
#include "components/spawner.hpp"

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

entt::entity GetTargetSpawnFor(Registry& aRegistry, PlayerID aID)
{
    auto    eliminated = aRegistry.view<Eliminated>();
    uint8_t playerSlot = 0;

    std::map<uint8_t, PlayerID> alive;
    for (const auto&& [entity, p] : aRegistry.view<Player>().each()) {
        if (!eliminated.contains(entity)) {
            alive.emplace(p.Slot, p.ID);
        }
        if (p.ID == aID) {
            playerSlot = p.Slot;
        }
    }

    if (alive.empty()) return entt::null;

    // Find the next alive slot after playerSlot (wrapping around)
    auto it = alive.upper_bound(playerSlot);
    if (it == alive.end()) it = alive.begin();

    for (const auto&& [entity, owner] : aRegistry.view<Spawner, Owner>().each()) {
        if (owner.ID == it->second) return entity;
    }

    return entt::null;
}
