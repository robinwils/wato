#include "systems/health.hpp"

#include "components/creep.hpp"
#include "components/health.hpp"
#include "core/sys/log.hpp"

void HealthSystem::Execute(Registry& aRegistry, [[maybe_unused]] std::uint32_t aTick)
{
    for (auto [entity, health, creep] : aRegistry.view<Health, Creep>().each()) {
        if (health.Health <= 0.0f) {
            WATO_INFO(aRegistry, "creep {} died (health: {})", entity, health.Health);
            aRegistry.destroy(entity);
        }
    }
}
