#include "input/input.hpp"

#include <fmt/ranges.h>
#include <reactphysics3d/collision/RaycastInfo.h>
#include <reactphysics3d/mathematics/Ray.h>

#include <optional>

#include "components/placement_mode.hpp"
#include "components/rigid_body.hpp"
#include "components/tile.hpp"
#include "components/transform3d.hpp"
#include "core/physics/physics.hpp"
#include "core/state.hpp"
#include "core/window.hpp"
#include "input/action.hpp"
#include "registry/registry.hpp"
#include "systems/input.hpp"

void InputSystem::operator()(Registry& aRegistry)
{
    const Input& input       = aRegistry.ctx().get<WatoWindow&>().GetInput();
    auto&        actionCtx   = aRegistry.ctx().get<ActionContextStack&>().front();
    auto&        buf         = aRegistry.ctx().get<GameStateBuffer&>();
    GameState&   latestState = buf.Latest();

    handleMouseMovement(aRegistry);

    const ActionsType& curActions = actionCtx.Bindings.ActionsFromInput(input);

    if (!curActions.empty()) {
        spdlog::trace("inserting latest {} actions: {}", curActions.size(), curActions);
        latestState.Actions.insert(latestState.Actions.end(), curActions.begin(), curActions.end());
    }
}

void InputSystem::handleMouseMovement(Registry& aRegistry)
{
    auto&       contextStack = aRegistry.ctx().get<ActionContextStack&>();
    const auto& currentCtx   = contextStack.front();
    auto&       window       = aRegistry.ctx().get<WatoWindow&>();
    auto&       phy          = aRegistry.ctx().get<Physics&>();
    glm::vec3   origin, end;

    for (auto&& [_, camera, tcam] : aRegistry.view<Camera, Transform3D>().each()) {
        std::tie(origin, end) = window.MouseUnproject(camera, tcam.Position);
    }

    switch (currentCtx.State) {
        case ActionContext::State::Placement: {
            if (std::optional<glm::vec3> intersect = phy.RayTerrainIntersection(origin, end);
                intersect) {
                window.SetMouseIntersect(*intersect);
                auto placementModeView = aRegistry.view<PlacementMode>();
                for (auto ghostTower : placementModeView) {
                    aRegistry.patch<Transform3D>(ghostTower, [&](Transform3D& aT) {
                        aT.Position.x = intersect->x;
                        aT.Position.z = intersect->z;
                    });
                }
            }
            break;
        }
        default:
            window.ResetMouseIntersect();
            break;
    }
}
