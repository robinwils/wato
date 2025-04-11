#include "systems/creep.hpp"

#include "components/creep.hpp"
#include "components/creep_spawn.hpp"
#include "components/health.hpp"
#include "components/transform3d.hpp"
#include "core/physics.hpp"

void CreepSystem::operator()(Registry& aRegistry, const float aDeltaTime)
{
    auto& phy = aRegistry.ctx().get<Physics&>();

    for (auto cmd : aRegistry.view<CreepSpawn>()) {
        auto  creep = aRegistry.create();
        auto& t     = aRegistry.emplace<Transform3D>(creep,
            glm::vec3(2.0f, 0.0f, 2.0f),
            glm::identity<glm::quat>(),
            glm::vec3(0.1f));

        aRegistry.emplace<Health>(creep, 100.0f);
        aRegistry.emplace<Creep>(creep, Creep::Simple);

        auto* body = phy.CreateRigidBody(creep,
            aRegistry,
            RigidBodyParams{.Type = rp3d::BodyType::DYNAMIC,
                .Transform        = t.ToRP3D(),
                .GravityEnabled   = false});
        phy.AddBoxCollider(body, rp3d::Vector3(0.35F, 0.65F, 0.35F), true);

        aRegistry.destroy(cmd);
    }
}
