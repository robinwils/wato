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
#include "core/pick_state.hpp"
#include "core/state.hpp"
#include "core/sys/log.hpp"
#include "core/window.hpp"
#include "input/action.hpp"
#include "registry/registry.hpp"
#include "systems/input.hpp"

void InputSystem::Execute(Registry& aRegistry, float aDelta)
{
    auto& window = GetSingletonComponent<WatoWindow&>(aRegistry);
    if (!window.GetInput().UiWantsMouse) {
        handleMouseMovement(aRegistry);
    }
    if (!window.GetInput().UiWantsKeyboard) {
        createActions(aRegistry, aDelta);
    }
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

    rp3d::Ray ray(ToRP3D(origin), ToRP3D(end));
    if (stack.GetState<PlacementState>() != nullptr) {
        if (std::optional<glm::vec3> intersect = phy.RayTerrainIntersection(ray);
            intersect) {
            auto placementModeView = aRegistry.view<PlacementMode>();

            aRegistry.ctx().get<PickState>().MouseWorldIntersect = *intersect;
            for (auto ghostTower : placementModeView) {
                aRegistry.patch<Transform3D>(ghostTower, [&](Transform3D& aT) {
                    aT.Position.x = intersect->x;
                    aT.Position.z = intersect->z;
                });
            }
        }
    } else {
        auto& state = GetSingletonComponent<PickState&>(aRegistry);

        if (state.MouseWorldIntersect.has_value()) {
            state.MouseWorldIntersect.reset();
        }

        handleMouseSelection(aRegistry, ray);
    }
}

void InputSystem::handleMouseSelection(Registry& aRegistry, const rp3d::Ray& aMouseRay)
{
    const Input& input = GetSingletonComponent<WatoWindow&>(aRegistry).GetInput();
    auto&        phy   = GetSingletonComponent<Physics&>(aRegistry);
    auto&        state = GetSingletonComponent<PickState&>(aRegistry);

     if (!input.PrevMouseState.IsKeyPressed(Mouse::Left)
        && input.MouseState.IsKeyPressed(Mouse::Left)) {
        state.SelectEndPos.reset();
        state.WorldStartPos.reset();

        if (std::optional<glm::vec3> intersect = phy.RayTerrainIntersection(aMouseRay); intersect) {
            state.WorldStartPos = intersect;
        }
    } else if (
        input.PrevMouseState.IsKeyPressed(Mouse::Left)
        && input.MouseState.IsKeyPressed(Mouse::Left)) {
        if (input.KeyboardState.IsKeyPressed(Keyboard::Escape)) {
            state.WorldStartPos.reset();
            state.SelectCurPos.reset();
            state.SelectEndPos.reset();
        } else {
            state.SelectCurPos = input.MouseState.Pos;
        }
    } else if (
        input.PrevMouseState.IsKeyPressed(Mouse::Left)
        && input.MouseState.IsKeyReleased(Mouse::Left)) {
        state.SelectCurPos.reset();
        state.SelectEndPos = input.MouseState.Pos;
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
