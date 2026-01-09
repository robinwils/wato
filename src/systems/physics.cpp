#include "systems/physics.hpp"

#include <bx/bx.h>

#include <cstring>
#include <entt/core/hashed_string.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "components/game.hpp"
#include "components/model_rotation_offset.hpp"
#include "components/rigid_body.hpp"
#include "components/tile.hpp"
#include "components/transform3d.hpp"
#include "core/physics/physics.hpp"
#include "core/state.hpp"
#include "registry/registry.hpp"
#include "systems/system_executor.hpp"

static constexpr float kTimeStep = 1.0f / 60.0f;

void PhysicsSystem::Execute(Registry& aRegistry, [[maybe_unused]] std::uint32_t aTick)
{
    auto& phy = aRegistry.ctx().get<Physics&>();

    // Update the Dynamics world with a constant time step (1/60s)
    phy.World()->update(kTimeStep);
}

void SimulationSystem::UpdateTransforms(Registry& aRegistry, const float aFactor)
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

void SimulationSystem::Execute(Registry& aRegistry, const float aDelta)
{
    auto& state     = aRegistry.ctx().get<GameStateBuffer&>();
    auto& instance  = aRegistry.ctx().get<GameInstance&>();
    auto& observers = aRegistry.ctx().get<Observers>();
    auto& fixedExec = aRegistry.ctx().get<FixedSystemExecutor>();

    instance.Accumulator += aDelta;

    // While there is enough accumulated time to take
    // one or several physics steps
    while (instance.Accumulator >= kTimeStep) {
        // Decrease the accumulated time
        instance.Accumulator -= kTimeStep;

        // Increment tick and run fixed timestep systems
        ++instance.Tick;
        fixedExec.Update(instance.Tick, &aRegistry);

        state.Push();
        state.Latest().Tick = instance.Tick;

        for (const entt::hashed_string& hash : observers) {
            auto* storage = aRegistry.storage(hash);

            if (storage == nullptr) {
                throw std::runtime_error(fmt::format("{} storage not initiated", hash.data()));
            }

            storage->clear();
        }
    }

    UpdateTransforms(aRegistry, instance.Accumulator / kTimeStep);
}
