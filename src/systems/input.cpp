#include "input/input.hpp"

#include <reactphysics3d/collision/RaycastInfo.h>
#include <reactphysics3d/mathematics/Ray.h>
#include <spdlog/fmt/bundled/ranges.h>

#include "components/placement_mode.hpp"
#include "components/rigid_body.hpp"
#include "components/tile.hpp"
#include "components/transform3d.hpp"
#include "core/physics.hpp"
#include "core/window.hpp"
#include "input/action.hpp"
#include "registry/registry.hpp"
#include "systems/input.hpp"

void InputSystem::operator()(Registry& aRegistry, const float aDeltaTime)
{
    const Input&   input         = aRegistry.ctx().get<WatoWindow&>().GetInput();
    auto&          actionCtx     = aRegistry.ctx().get<ActionContextStack&>().front();
    auto&          abuf          = aRegistry.ctx().get<ActionBuffer&>();
    PlayerActions& latestActions = abuf.Latest();
    const ActionBindings::actions_type& curActions = actionCtx.Bindings.ActionsFromInput(input);

    if (!curActions.empty()) {
        spdlog::info("inserting latest {} actions: {}", curActions.size(), curActions);
        latestActions.Actions.insert(
            latestActions.Actions.end(),
            curActions.begin(),
            curActions.end());
    }
    handleMouseMovement(aRegistry, aDeltaTime);
}

void InputSystem::handleMouseMovement(Registry& aRegistry, const float aDeltaTime)
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
        case ActionContext::State::Default:
            break;
        case ActionContext::State::Placement:
            WorldRaycastCallback raycastCb;

            // rp3d wants a start and end vector3, multiply normalized dir by 1000 (arbitrary)
            // to get a point far enough for the ray to intersect.
            rp3d::Ray ray(ToRP3D(origin), ToRP3D(end));
            phy.World()->raycast(ray, &raycastCb, Category::Terrain);
            if (!raycastCb.Hits.empty()) {
                auto placementModeView = aRegistry.view<PlacementMode>();
                for (auto ghostTower : placementModeView) {
                    aRegistry.patch<Transform3D>(ghostTower,
                        [raycastCb, &aRegistry, ghostTower](Transform3D& aT) {
                            aT.Position.x = raycastCb.Hits[0].x;
                            aT.Position.z = raycastCb.Hits[0].z;
                            aRegistry.patch<RigidBody>(ghostTower,
                                [aT](RigidBody& aRb) { aRb.RigidBody->setTransform(aT.ToRP3D()); });
                        });
                }
            }
            break;
    }
}
