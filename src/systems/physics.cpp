#include "components/physics.hpp"

#include <cstdint>
#include <cstring>
#include <entt/core/hashed_string.hpp>

#include "bgfx/bgfx.h"
#include "bgfx/defines.h"
#include "bx/bx.h"
#include "components/rigid_body.hpp"
#include "components/transform3d.hpp"
#include "config.h"
#include "core/cache.hpp"
#include "core/registry.hpp"
#include "reactphysics3d/utils/DebugRenderer.h"
#include "renderer/material.hpp"
#include "systems/systems.hpp"

void physicsSystem(Registry& aRegistry, double aDeltaTime)
{
    auto& phy = aRegistry.ctx().get<Physics>();

    // Constant physics time step, TODO: as static const for now
    static const rp3d::decimal timeStep    = 1.0F / 60.0F;
    static rp3d::decimal       accumulator = 0.0F;

    // Add the time difference in the accumulator
    accumulator += aDeltaTime;

    // While there is enough accumulated time to take
    // one or several physics steps
    while (accumulator >= timeStep) {
        // Update the Dynamics world with a constant time step
        phy.World->update(timeStep);

        // Decrease the accumulated time
        accumulator -= timeStep;
    }

    // Compute the time interpolation factor
    rp3d::decimal factor = accumulator / timeStep;

    // update transforms
    for (auto&& [entity, t, rb] : aRegistry.view<Transform3D, RigidBody>().each()) {
        auto updatedTransform = rb.rigid_body->getTransform();
        auto interpolatedTransform =
            reactphysics3d::Transform::interpolateTransforms(t.ToRP3D(), updatedTransform, factor);
        t.FromRP3D(interpolatedTransform);
    }
}

#if WATO_DEBUG
static bool vInit = false;
struct PosColor {
    float    MX;
    float    MY;
    float    MZ;
    uint32_t MRgba;

    static void Init()
    {
        if (vInit) {
            return;
        }
        msLayout.begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
            .end();
        vInit = true;
    }

    static bgfx::VertexLayout msLayout;
};
bgfx::VertexLayout PosColor::msLayout;

void physicsDebugRenderSystem(Registry& aRegistry)
{
    auto&                      phy           = aRegistry.ctx().get<Physics>();
    rp3d::DebugRenderer const& debugRenderer = phy.World->getDebugRenderer();

    PosColor::Init();

    auto nTri   = debugRenderer.getNbTriangles();
    auto nLines = debugRenderer.getNbLines();

    auto  debugShader = WATO_PROGRAM_CACHE["simple"_hs];
    auto* debugMat    = new Material(debugShader);
    if (nTri > 0) {
        auto state = BGFX_STATE_DEFAULT;
        if (3 * nTri == bgfx::getAvailTransientVertexBuffer(3 * nTri, PosColor::msLayout)) {
            bgfx::TransientVertexBuffer vb{};
            bgfx::allocTransientVertexBuffer(&vb, 3 * nTri, PosColor::msLayout);

            bx::memCopy(vb.data, debugRenderer.getTrianglesArray(), 3 * nTri * sizeof(PosColor));

            bgfx::setState(state);
            bgfx::setVertexBuffer(0, &vb);
            bgfx::submit(0, debugMat->Program(), bgfx::ViewMode::Default);
        }
    }
    if (nLines > 0) {
        auto state = BGFX_STATE_DEFAULT | BGFX_STATE_PT_LINES;
        if (2 * nLines == bgfx::getAvailTransientVertexBuffer(2 * nLines, PosColor::msLayout)) {
            bgfx::TransientVertexBuffer vb{};
            bgfx::allocTransientVertexBuffer(&vb, 2 * nLines, PosColor::msLayout);

            bx::memCopy(vb.data, debugRenderer.getLinesArray(), 2 * nLines * sizeof(PosColor));

            bgfx::setState(state);
            bgfx::setVertexBuffer(0, &vb);
            bgfx::submit(0, debugMat->Program(), bgfx::ViewMode::Default);
        }
    }
}
#endif
