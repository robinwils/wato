#include "systems/economy.hpp"

#include "components/game.hpp"
#include "components/player.hpp"
#include "core/net/enet_server.hpp"
#include "core/net/net.hpp"
#include "core/sys/log.hpp"
#include "registry/registry.hpp"

void EconomySystem::PeriodicExecute(Registry& aRegistry, [[maybe_unused]] std::uint32_t aTick)
{
    auto& income = aRegistry.ctx().get<CommonIncome&>();

    for (auto [entity, player, gold] : aRegistry.view<Player, Gold>().each()) {
        gold.Balance += income.Value;
        WATO_DBG(aRegistry, "player {} income +{}, balance {}", player.ID, income.Value, gold.Balance);

        if (auto* server = aRegistry.ctx().find<ENetServer>()) {
            server->BroadcastResponse(
                GetPlayerIDs(aRegistry),
                PacketType::Ack,
                GetSingletonComponent<GameInstance&>(aRegistry).Tick,
                GoldUpdateResponse{
                    .Player  = player.ID,
                    .Balance = gold.Balance,
                });
        }
    }
}
