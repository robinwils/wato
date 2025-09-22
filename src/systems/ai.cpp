#include "systems/ai.hpp"

#include <bgfx/bgfx.h>
#include <bgfx/defines.h>
#include <spdlog/spdlog.h>

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include "components/creep.hpp"
#include "components/rigid_body.hpp"
#include "components/transform3d.hpp"
#include "components/velocity.hpp"
#include "core/graph.hpp"

void AiSystem::operator()(Registry& aRegistry, const float aDeltaTime)
{
    auto& graph = aRegistry.ctx().get<Graph>();

    for (auto&& [_, creep, t, rb, v] :
         aRegistry.view<Creep, Transform3D, RigidBody, Velocity>().each()) {
        if (auto next = graph.GetNextCell(GraphCell::FromWorldPoint(t.Position))) {
            auto dir = glm::normalize(next->ToWorld() - t.Position);

            t.Position += v.Velocity * dir;
            // rb.Body->applyWorldForceAtCenterOfMass(ToRP3D(dir * v.Velocity));
            rb.Body->setTransform(t.ToRP3D());
            spdlog::trace(
                "next: {}({}), velocity: {}, force: {} mix: {}",
                *next,
                glm::to_string(next->ToWorld()),
                v.Velocity,
                ToRP3D(dir * v.Velocity),
                glm::to_string(t.Position));
        }
    }
}
