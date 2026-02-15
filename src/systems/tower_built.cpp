#include "systems/tower_built.hpp"

#include <bgfx/bgfx.h>
#include <bx/bx.h>

#include <set>
#include <stdexcept>

#include "components/health.hpp"
#include "components/imgui.hpp"
#include "components/player.hpp"
#include "components/rigid_body.hpp"
#include "components/spawner.hpp"
#include "core/graph.hpp"
#include "core/net/enet_server.hpp"

using namespace entt::literals;

void TowerBuiltSystem::Execute(Registry& aRegistry, [[maybe_unused]] std::uint32_t aTick)
{
    auto& phy     = aRegistry.ctx().get<Physics>();
    auto* storage = aRegistry.storage("tower_built_observer"_hs);

    if (storage == nullptr) {
        throw std::runtime_error("tower_built_observer storage not initialized");
    }

    if (storage->empty()) {
        return;
    }
    WATO_TRACE(aRegistry, "got {} towers built", storage->size());

    // Collect which player graphs need path recomputation
    std::set<PlayerID> dirtyPlayers;

    if (aRegistry.ctx().contains<PlayerGraphMap>()) {
        auto& graphMap = aRegistry.ctx().get<PlayerGraphMap>();

        for (auto tower : *storage) {
            auto& rb = aRegistry.get<RigidBody>(tower);
            auto* owner = aRegistry.try_get<Owner>(tower);
            if (owner) {
                auto it = graphMap.find(owner->ID);
                if (it != graphMap.end()) {
                    phy.ToggleObstacle(rb.Body->getCollider(0), it->second, true);
                    dirtyPlayers.insert(owner->ID);
                }
            }
        }

        for (PlayerID pid : dirtyPlayers) {
            auto it = graphMap.find(pid);
            if (it == graphMap.end()) continue;
            auto& graph = it->second;

            for (auto&& [e, player, transform] : aRegistry.view<Player, Transform3D>().each()) {
                if (player.ID == pid) {
                    graph.ComputePaths(GraphCell::FromWorldPoint(transform.Position));
                    WATO_TRACE(aRegistry, "paths updated for player {}", pid);
                    break;
                }
            }
            graph.GridDirty = true;
        }
    } else if (aRegistry.ctx().contains<Graph>()) {
        // Client path: single graph in context
        auto& graph = aRegistry.ctx().get<Graph>();
        for (auto tower : *storage) {
            auto& rb = aRegistry.get<RigidBody>(tower);
            phy.ToggleObstacle(rb.Body->getCollider(0), graph, true);
        }
        graph.GridDirty = true;
    }
}
