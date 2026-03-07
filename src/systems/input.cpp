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
#include "core/sys/log.hpp"
#include "core/window.hpp"
#include "input/action.hpp"
#include "registry/registry.hpp"
#include "systems/input.hpp"

void InputSystem::Execute(Registry& aRegistry, float aDelta)
{
    handleMouseMovement(aRegistry);
    createActions(aRegistry, aDelta);
}

void InputSystem::handleMouseMovement(Registry& aRegistry)
{
    auto& stack  = aRegistry.ctx().get<ActionContextStack&>();
    auto& window = aRegistry.ctx().get<WatoWindow&>();
    auto& phy    = aRegistry.ctx().get<Physics&>();

    glm::vec3 origin, end;

    for (auto&& [_, camera, tcam] : aRegistry.view<Camera, Transform3D>().each()) {
        std::tie(origin, end) = window.MouseUnproject(camera, tcam.Position);
    }

    if (stack.GetState<PlacementState>()) {
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
    } else {
        window.ResetMouseIntersect();
    }
}

void InputSystem::createActions(Registry& aRegistry, float aDelta)
{
    const Input& input = aRegistry.ctx().get<WatoWindow&>().GetInput();
    auto&        stack = aRegistry.ctx().get<ActionContextStack&>();
    auto&        frameBuf = aRegistry.ctx().get<FrameActionBuffer&>();
    auto&        gameBuf  = aRegistry.ctx().get<GameStateBuffer&>();

    stack.CurrentBindings().Visit([&](ActionBinding& aBinding) {
        if (!aBinding.KeyState.IsTriggered(input)) return;

        Action action = aBinding.Action;
        action.AddExtraInputInfo(input);

        WATO_TRACE(aRegistry, "got action triggered: {}", action);

        if (action.IsFrameTime()) {
            frameBuf.push_back(action);
        } else {
            gameBuf.Latest().Actions.push_back(action);
        }
    });

    if (!input.UiWantsMouse) {
        if (input.MouseState.Scroll.y > 0) {
            frameBuf.push_back(Action{.Payload = MovePayload{.Direction = MoveDirection::Down}});
        } else if (input.MouseState.Scroll.y < 0) {
            frameBuf.push_back(Action{.Payload = MovePayload{.Direction = MoveDirection::Up}});
        }
    }
}
