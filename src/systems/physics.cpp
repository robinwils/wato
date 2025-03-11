#include <reactphysics3d/reactphysics3d.h>

#include <cstring>
#include <entt/core/hashed_string.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "bgfx/bgfx.h"
#include "bgfx/defines.h"
#include "bx/bx.h"
#include "core/cache.hpp"
#include "core/registry.hpp"
#include "core/sys.hpp"
#include "entt/core/fwd.hpp"
#include "renderer/material.hpp"
#include "renderer/primitive.hpp"
#include "systems/systems.hpp"

void physicsSystem(Registry& registry, double delta_time)
{
    auto* phy_world = registry.ctx().get<rp3d::PhysicsWorld*>();

    // Constant physics time step, TODO: as static const for now
    static const double time_step   = 1.0f / 60.0f;
    static double       accumulator = 0.0f;

    // Add the time difference in the accumulator
    accumulator += delta_time;

    // While there is enough accumulated time to take
    // one or several physics steps
    while (accumulator >= time_step) {
        // Update the Dynamics world with a constant time step
        phy_world->update(time_step);

        // Decrease the accumulated time
        accumulator -= time_step;
    }
}

#if WATO_DEBUG
static bool v_init = false;
struct PosColor {
    float    m_x;
    float    m_y;
    float    m_z;
    uint32_t m_rgba;

    static void init()
    {
        if (v_init) {
            return;
        }
        ms_layout.begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
            .end();
        v_init = true;
    }

    static bgfx::VertexLayout ms_layout;
};
bgfx::VertexLayout PosColor::ms_layout;

void physicsDebugRenderSystem(Registry& registry)
{
    auto*                phy_world      = registry.ctx().get<rp3d::PhysicsWorld*>();
    rp3d::DebugRenderer& debug_renderer = phy_world->getDebugRenderer();

    PosColor::init();

    auto n_tri   = debug_renderer.getNbTriangles();
    auto n_lines = debug_renderer.getNbLines();

    auto  debug_shader = PROGRAM_CACHE["simple"_hs];
    auto* debug_mat    = new Material(debug_shader);
    if (n_tri > 0) {
        auto state = BGFX_STATE_DEFAULT;
        if (3 * n_tri == bgfx::getAvailTransientVertexBuffer(3 * n_tri, PosColor::ms_layout)) {
            bgfx::TransientVertexBuffer vb;
            bgfx::allocTransientVertexBuffer(&vb, 3 * n_tri, PosColor::ms_layout);

            bx::memCopy(vb.data, debug_renderer.getTrianglesArray(), 3 * n_tri * sizeof(PosColor));

            bgfx::setState(state);
            bgfx::setVertexBuffer(0, &vb);
            bgfx::submit(0, debug_mat->shader->program(), bgfx::ViewMode::Default);
        }
    }
    if (n_lines > 0) {
        auto state = BGFX_STATE_DEFAULT | BGFX_STATE_PT_LINES;
        if (2 * n_lines == bgfx::getAvailTransientVertexBuffer(2 * n_lines, PosColor::ms_layout)) {
            bgfx::TransientVertexBuffer vb;
            bgfx::allocTransientVertexBuffer(&vb, 2 * n_lines, PosColor::ms_layout);

            bx::memCopy(vb.data, debug_renderer.getLinesArray(), 2 * n_lines * sizeof(PosColor));

            bgfx::setState(state);
            bgfx::setVertexBuffer(0, &vb);
            bgfx::submit(0, debug_mat->shader->program(), bgfx::ViewMode::Default);
        }
    }
}
#endif
