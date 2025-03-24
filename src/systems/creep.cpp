#include "systems/creep.hpp"

#include "components/creep_spawn.hpp"
#include "components/health.hpp"

void CreepSystem::operator()(Registry& aRegistry, const float aDeltaTime)
{
    for (auto cmd : aRegistry.view<CreepSpawn>()) {
        auto creep = aRegistry.create();
        aRegistry.emplace<Health>(creep, 100.0f);
        aRegistry.destroy(cmd);
    }
}
