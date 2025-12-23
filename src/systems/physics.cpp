#include "systems/physics.hpp"

#include <bx/bx.h>

#include <cstring>
#include <entt/core/hashed_string.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "components/creep.hpp"
#include "components/model_rotation_offset.hpp"
#include "components/projectile.hpp"
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

            // Orient entities with movement direction (creeps, projectiles, etc.)
            glm::vec3 direction = rb.Params.Direction;
            if (glm::length(direction) > 0.001f) {
                // Calculate rotation to align entity with movement direction
                glm::vec3 forward = glm::normalize(direction);
                glm::vec3 up      = glm::vec3(0.0f, 1.0f, 0.0f);

                // Avoid gimbal lock when direction is parallel to up vector
                if (glm::abs(glm::dot(forward, up)) > 0.99f) {
                    up = glm::vec3(1.0f, 0.0f, 0.0f);
                }

                glm::vec3 right = glm::normalize(glm::cross(up, forward));
                glm::vec3 newUp = glm::cross(forward, right);

                glm::quat baseOrientation = glm::quatLookAt(forward, newUp);

                // Apply model-specific rotation offset if present
                if (auto* offset = aRegistry.try_get<ModelRotationOffset>(entity)) {
                    aT.Orientation = baseOrientation * offset->Offset;
                } else {
                    aT.Orientation = baseOrientation;
                }
            }
        });
    }
}
