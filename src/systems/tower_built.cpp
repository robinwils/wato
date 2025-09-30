#include "systems/tower_built.hpp"

#include <bgfx/bgfx.h>
#include <bx/bx.h>

#include <stdexcept>

#include "components/health.hpp"
#include "components/imgui.hpp"
#include "components/rigid_body.hpp"
#include "components/spawner.hpp"
#include "core/graph.hpp"
#include "resource/cache.hpp"

using namespace entt::literals;

void TowerBuiltSystem::operator()(Registry& aRegistry, const float aDeltaTime)
{
    auto& graph   = aRegistry.ctx().get<Graph>();
    auto* storage = aRegistry.storage("tower_built_observer"_hs);

    if (storage == nullptr) {
        throw std::runtime_error("tower_built_observer storage not initialized");
    }

    if (storage->size() == 0) {
        return;
    }
    spdlog::trace("got {} towers built", storage->size());

    for (auto tower : *storage) {
        auto& rb = aRegistry.get<RigidBody>(tower);

        rb.Body->getCollider(0)->setIsSimulationCollider(true);
        rb.Body->getCollider(0)->setCollisionCategoryBits(Category::Entities);
        rb.Body->getCollider(0)->setCollideWithMaskBits(
            Category::Terrain | Category::PlacementGhostTower);
        rb.Body->setType(rp3d::BodyType::STATIC);

        // FIXME: this sometimes does some weird entt crash saying slot is not available
        aRegistry.emplace<Health>(tower, 100.0F);
        aRegistry.remove<ImguiDrawable>(tower);

        ToggleObstacle(rb.Body->getCollider(0), aRegistry.ctx().get<Graph>(), true);
    }

    // FIXME: need to think about player ownership and how to handle only updating the correct
    // player's grid
    for (auto&& [base, transform] : aRegistry.view<Base, Transform3D>().each()) {
        graph.ComputePaths(GraphCell::FromWorldPoint(transform.Position));
        spdlog::trace("paths updated");
        spdlog::debug("{}", aRegistry.ctx().get<Graph>());
        break;
    }

    BX_ASSERT(
        graph.GridLayout().size() == graph.Width() * graph.Height(),
        "incorrect graph data length");
    entt::resource<bgfx::TextureHandle> handle = aRegistry.ctx().get<TextureCache>()["grid_tex"_hs];
    bgfx::updateTexture2D(
        handle,
        0,
        0,
        0,
        0,
        graph.Width(),
        graph.Height(),
        bgfx::copy(graph.GridLayout().data(), graph.Width() * graph.Height()));
}
