#include "systems/physics.hpp"

#include <bx/bx.h>

#include <cstring>
#include <entt/core/hashed_string.hpp>

#include "components/rigid_body.hpp"
#include "components/transform3d.hpp"
#include "core/physics.hpp"
#include "registry/registry.hpp"

void PhysicsSystem::operator()(Registry& aRegistry, const float aDeltaTime)
{
    auto& phy = aRegistry.ctx().get<Physics&>();

    // Constant physics time step, TODO: as static const for now
    static const rp3d::decimal timeStep    = 1.0F / 60.0F;
    static rp3d::decimal       accumulator = 0.0F;

    // Add the time difference in the accumulator
    accumulator += aDeltaTime;

    // While there is enough accumulated time to take
    // one or several physics steps
    while (accumulator >= timeStep) {
        // Update the Dynamics world with a constant time step
        phy.World()->update(timeStep);

        // Decrease the accumulated time
        accumulator -= timeStep;
    }

    // Compute the time interpolation factor
    rp3d::decimal factor = accumulator / timeStep;

    // update transforms
    for (auto&& [entity, t, rb] : aRegistry.view<Transform3D, RigidBody>().each()) {
        auto updatedTransform = rb.rigid_body->getTransform();
        auto interpolatedTransform =
            reactphysics3d::Transform::interpolateTransforms(t.ToRP3D(), updatedTransform, factor);
        t.FromRP3D(interpolatedTransform);
    }
}
