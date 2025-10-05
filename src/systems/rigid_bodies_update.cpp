#include "systems/rigid_bodies_update.hpp"

#include <bgfx/bgfx.h>
#include <bx/bx.h>

#include <stdexcept>

#include "components/health.hpp"
#include "components/imgui.hpp"
#include "components/rigid_body.hpp"
#include "components/spawner.hpp"
#include "components/transform3d.hpp"
#include "core/graph.hpp"
#include "resource/cache.hpp"

using namespace entt::literals;

void RigidBodiesUpdateSystem::operator()(Registry& aRegistry, const float aDeltaTime)
{
    auto& physics          = aRegistry.ctx().get<Physics>();
    auto* rbStorage        = aRegistry.storage("rigid_bodies_observer"_hs);
    auto* placementStorage = aRegistry.storage("placement_mode_observer"_hs);

    if (rbStorage == nullptr) {
        throw std::runtime_error("rigid_bodies_observer storage not initialized");
    }
    if (placementStorage == nullptr) {
        throw std::runtime_error("rigid_bodies_observer storage not initialized");
    }

    for (auto& e : *rbStorage) {
        auto& rb = aRegistry.get<RigidBody>(e);
        auto& c  = aRegistry.get<Collider>(e);
        auto& t  = aRegistry.get<Transform3D>(e);

        if (!rb.Body) {
            rb.Body  = physics.CreateRigidBody(rb.Params, t);
            c.Handle = physics.AddCollider(rb.Body, c.Params);
        }

        if (rb.Params.Data != rb.Body->getUserData()) {
            rb.Body->setUserData(rb.Params.Data);
        }
    }

    if (placementStorage->size() == 0) {
        return;
    }

    // spdlog::trace("got {} transforms updated", placementStorage->size());

    for (auto& e : *placementStorage) {
    }
}
