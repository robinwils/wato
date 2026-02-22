#include "systems/health.hpp"

#include <entt/entity/fwd.hpp>

#include "components/creep.hpp"
#include "components/game.hpp"
#include "components/health.hpp"
#include "components/player.hpp"
#include "core/net/enet_server.hpp"
#include "core/net/net.hpp"
#include "core/sys/log.hpp"
#include "registry/registry.hpp"

void HealthSystem::Execute(Registry& aRegistry, [[maybe_unused]] std::uint32_t aTick)
{
    for (auto [entity, health, creep] : aRegistry.view<Health, Creep>().each()) {
        if (health.Health <= 0.0f) {
            WATO_INFO(aRegistry, "creep {} died (health: {})", entity, health.Health);
            aRegistry.destroy(entity);
        }
    }

    auto  group   = aRegistry.group<Player>(entt::get<Health>, entt::exclude<Eliminated>);
    auto& ranking = aRegistry.ctx().get<std::vector<PlayerID>>("ranking"_hs);
    auto* server  = aRegistry.ctx().find<ENetServer>();

    auto& instance = aRegistry.ctx().get<GameInstance&>();

    for (auto [entity, player, health] : group.each()) {
        if (health.Health <= 0.0f) {
            WATO_INFO(aRegistry, "player {} lost (health: {})", entity, health.Health);

            aRegistry.emplace<Eliminated>(entity);
            if (server) {
                ranking.push_back(player.ID);

                server->BroadcastResponse(
                    GetPlayerIDs(aRegistry),
                    PacketType::Ack,
                    instance.Tick,
                    PlayerEliminatedResponse{.PlayerID = player.ID, .Ranking = ranking});
            }
        }
    }

    if (group.size() <= 1 && !instance.IsOver) {
        instance.IsOver = true;

        // Append survivors so ranking = [first_out, ..., winner]
        // reverse_view on the client gives rank 1 = winner
        for (auto [entity, player, health] : group.each()) {
            ranking.push_back(player.ID);
        }

        if (server) {
            server->BroadcastResponse(
                GetPlayerIDs(aRegistry),
                PacketType::Ack,
                instance.Tick,
                GameEndResponse{.Ranking = ranking});
        }
    }
}
