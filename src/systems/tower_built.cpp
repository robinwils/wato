#include "systems/tower_built.hpp"

#include <bgfx/bgfx.h>

#include "components/health.hpp"
#include "components/imgui.hpp"
#include "components/rigid_body.hpp"
#include "core/graph.hpp"

using namespace entt::literals;

void TowerBuiltSystem::operator()(Registry& aRegistry, const float aDeltaTime)
{
    auto& graph   = aRegistry.ctx().get<Graph>();
    auto& storage = aRegistry.storage<entt::reactive>("tower_built_observer"_hs);

    for (auto tower : storage) {
        auto& rb = aRegistry.get<RigidBody>(tower);

        rb.Body->getCollider(0)->setIsSimulationCollider(true);
        rb.Body->getCollider(0)->setCollisionCategoryBits(Category::Entities);
        rb.Body->getCollider(0)->setCollideWithMaskBits(
            Category::Terrain | Category::PlacementGhostTower);
        rb.Body->setType(rp3d::BodyType::STATIC);

        aRegistry.emplace<Health>(tower, 100.0F);
        aRegistry.remove<PlacementMode>(tower);
        aRegistry.remove<ImguiDrawable>(tower);

        ToggleObstacle(rb.Body->getCollider(0), aRegistry.ctx().get<Graph>(), true);
    }

    bgfx::updateTexture2D(
        aRegistry.ctx().get<bgfx::TextureHandle>("grid_tex"_hs),
        0,
        0,
        0,
        0,
        graph.Width,
        graph.Height,
        bgfx::copy(graph.GridLayout().data(), graph.Width * graph.Height));

    storage.clear();
}
