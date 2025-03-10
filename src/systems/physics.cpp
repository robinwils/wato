#include <reactphysics3d/reactphysics3d.h>

#include "core/registry.hpp"
#include "core/sys.hpp"

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

void physicsDebugRenderSystem(Registry& registry)
{
    auto*                phy_world      = registry.ctx().get<rp3d::PhysicsWorld*>();
    rp3d::DebugRenderer& debug_renderer = phy_world->getDebugRenderer();

    auto n_lines = debug_renderer.getNbLines();
    auto n_tri   = debug_renderer.getNbTriangles();

    // auto triangles = debug_renderer.getTriangles();
    if (n_lines > 0 || n_tri > 0) {
        DBG("physics: %d debug lines and %d debug triangles to draw", n_lines, n_tri);
    }
}
