#include "systems/physics.hpp"

#include <bx/bx.h>

#include <cstring>
#include <entt/core/hashed_string.hpp>

#include "components/rigid_body.hpp"
#include "components/tile.hpp"
#include "components/transform3d.hpp"
#include "core/physics/physics.hpp"
#include "registry/registry.hpp"

void PhysicsSystem::operator()(Registry& aRegistry, const float aDeltaTime)
{
    auto& phy = aRegistry.ctx().get<Physics&>();

    // Update the Dynamics world with a constant time step
    phy.World()->update(aDeltaTime);
}

void UpdateTransformsSytem::operator()(Registry& aRegistry, const float aFactor)
{
    // update transforms
    for (auto&& [entity, t, rb] :
         aRegistry.view<Transform3D, RigidBody>(entt::exclude<Tile>).each()) {
        if (!rb.Body) {
            continue;
        }
        auto updatedTransform = rb.Body->getTransform();
        auto interpolatedTransform =
            reactphysics3d::Transform::interpolateTransforms(t.ToRP3D(), updatedTransform, aFactor);

        aRegistry.patch<Transform3D>(entity, [&](Transform3D& aT) {
            aT.FromRP3D(interpolatedTransform);
        });
        // t.FromRP3D(interpolatedTransform);
    }
}
