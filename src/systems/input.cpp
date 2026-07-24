#include "input/input.hpp"

#include <fmt/ranges.h>
#include <reactphysics3d/collision/RaycastInfo.h>
#include <reactphysics3d/mathematics/Ray.h>

#include <optional>

#include "core/pick_state.hpp"
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
    auto& stack  = GetSingletonComponent<ActionContextStack&>(aRegistry);
    auto& window = GetSingletonComponent<WatoWindow&>(aRegistry);
    auto& phy    = GetSingletonComponent<Physics&>(aRegistry);

    glm::vec3 origin;
    glm::vec3 end;

    for (auto&& [_, camera, tcam] : aRegistry.view<Camera, Transform3D>().each()) {
        std::tie(origin, end) = window.MouseUnproject(camera, tcam.Position);
    }

    if (stack.GetState<PlacementState>()) {
        if (std::optional<glm::vec3> intersect = phy.RayTerrainIntersection(origin, end);
            intersect) {
            aRegistry.ctx().get<PickState>().MouseWorldIntersect = *intersect;
            auto placementModeView                               = aRegistry.view<PlacementMode>();
            for (auto ghostTower : placementModeView) {
                aRegistry.patch<Transform3D>(ghostTower, [&](Transform3D& aT) {
                    aT.Position.x = intersect->x;
                    aT.Position.z = intersect->z;
                });
            }
        }
    } else {
        GetSingletonComponent<PickState&>(aRegistry).MouseWorldIntersect.reset();
    }
}

void InputSystem::createActions(Registry& aRegistry, float aDelta)
{
    const Input& input    = GetSingletonComponent<WatoWindow&>(aRegistry).GetInput();
    auto&        stack    = GetSingletonComponent<ActionContextStack&>(aRegistry);
    auto&        frameBuf = GetSingletonComponent<FrameActionBuffer&>(aRegistry);
    auto&        gameBuf  = GetSingletonComponent<GameStateBuffer&>(aRegistry);

    stack.CurrentBindings().Visit([&](ActionBinding& aBinding) {
        if (!aBinding.KeyState.IsTriggered(input)) return;

        Action action = aBinding.Action;
        action.AddExtraInputInfo(GetSingletonComponent<PickState>(aRegistry).MouseWorldIntersect);

        mLogger->trace("got action triggered: {}", action);

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
