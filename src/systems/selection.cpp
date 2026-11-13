#include "systems/selection.hpp"

#include <bx/bounds.h>
#include <bx/math.h>
#include <spdlog/spdlog.h>

#include "components/selectable.hpp"
#include "core/math.hpp"
#include "core/pick_state.hpp"
#include "core/sys/log.hpp"
#include "core/window.hpp"

void SelectionSystem::Execute(Registry& aRegistry, [[maybe_unused]] std::uint32_t aTick)
{
    auto& window = aRegistry.ctx().get<WatoWindow&>();
    auto& state  = GetSingletonComponent<PickState&>(aRegistry);

    if (!state.SelectEndPos) {
        WATO_TRACE(aRegistry, "no user box selection");
        return;
    }

    Camera      cam{};
    Transform3D camT;

    for (auto&& [entity, camera, t3d] : aRegistry.view<Camera, Transform3D>().each()) {
        cam  = camera;
        camT = t3d;
        break;
    }

    auto selectedV   = aRegistry.view<Selected>();
    auto selectableV = aRegistry.view<Selectable>();

    for (auto&& entity : selectableV) {
        if (auto* t3d = aRegistry.try_get<Transform3D>(entity)) {
            glm::vec3 screen = window.ProjectPosition(t3d->Position, cam, camT.Position);
            if (selectedV->contains(entity) && !state.IsInSelection(screen)) {
                aRegistry.remove<Selected>(entity);
            } else if (!selectedV->contains(entity) && state.IsInSelection(screen)) {
                aRegistry.emplace<Selected>(entity);
            }
        }
    }
}
