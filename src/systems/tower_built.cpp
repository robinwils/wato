#include "systems/tower_built.hpp"

#include <bgfx/bgfx.h>
#include <bx/bx.h>

#include <stdexcept>

#include "components/health.hpp"
#include "components/imgui.hpp"
#include "components/rigid_body.hpp"
#include "components/spawner.hpp"
#include "core/graph.hpp"

using namespace entt::literals;

void TowerBuiltSystem::operator()(Registry& aRegistry)
{
    auto& graph   = aRegistry.ctx().get<Graph>();
    auto& phy     = aRegistry.ctx().get<Physics>();
    auto* storage = aRegistry.storage("tower_built_observer"_hs);

    if (storage == nullptr) {
        throw std::runtime_error("tower_built_observer storage not initialized");
    }

    if (storage->empty()) {
        return;
    }
    WATO_TRACE(aRegistry, "got {} towers built", storage->size());

    for (auto tower : *storage) {
        auto& rb = aRegistry.get<RigidBody>(tower);

        phy.ToggleObstacle(rb.Body->getCollider(0), aRegistry.ctx().get<Graph>(), true);
    }

    // FIXME: need to think about player ownership and how to handle only updating the correct
    // player's grid
    for (auto&& [base, transform] : aRegistry.view<Base, Transform3D>().each()) {
        graph.ComputePaths(GraphCell::FromWorldPoint(transform.Position));
        WATO_TRACE(aRegistry, "paths updated");
        WATO_DBG(aRegistry, "{}", aRegistry.ctx().get<Graph>());
        break;
    }
    graph.GridDirty = true;
}
