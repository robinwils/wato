#include "systems/projectile.hpp"

#include <glm/geometric.hpp>

#include "components/health.hpp"
#include "components/projectile.hpp"
#include "components/rigid_body.hpp"
#include "components/transform3d.hpp"
#include "core/sys/log.hpp"

void ProjectileSystem::operator()(Registry& aRegistry, float aDeltaTime)
{
    for (auto&& [projectileEntity, projectile, transform, rb] :
         aRegistry.view<Projectile, Transform3D, RigidBody>().each()) {
        if (!aRegistry.valid(projectile.Target)) {
            aRegistry.destroy(projectileEntity);
            WATO_TRACE(aRegistry, "destroyed projectile {} (invalid target)", projectileEntity);
            continue;
        }

        auto* targetTransform = aRegistry.try_get<Transform3D>(projectile.Target);
        if (!targetTransform) {
            aRegistry.destroy(projectileEntity);
            WATO_TRACE(aRegistry, "destroyed projectile {} (dead target)", projectileEntity);
            continue;
        }

        aRegistry.patch<RigidBody>(projectileEntity, [&](RigidBody& aBody) {
            aBody.Params.Direction = glm::normalize(targetTransform->Position - transform.Position);
        });
    }
}
