#include "systems/ai.hpp"

#include <bgfx/bgfx.h>
#include <bgfx/defines.h>
#include <spdlog/spdlog.h>

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include "components/creep.hpp"
#include "components/path.hpp"
#include "components/rigid_body.hpp"
#include "components/transform3d.hpp"
#include "components/velocity.hpp"
#include "core/graph.hpp"
#include "core/sys/log.hpp"

void AiSystem::operator()(Registry& aRegistry)
{
    auto& graph = aRegistry.ctx().get<Graph>();

    for (auto&& [_, creep, t, rb, p] :
         aRegistry.view<Creep, Transform3D, RigidBody, Path>().each()) {
        auto c = GraphCell::FromWorldPoint(t.Position);
        if (!p.NextCell || c == p.NextCell) {
            p.NextCell = graph.GetNextCell(c);
            p.LastFrom = c;
            spdlog::trace("set next cell = {} and last from = {}", p.NextCell, p.LastFrom);
        } else if (p.NextCell != graph.GetNextCell(p.LastFrom)) {
            // the path has probably been updated (tower built)
            spdlog::trace("path updated, setting next cell = ", graph.GetNextCell(p.LastFrom));
            p.NextCell = graph.GetNextCell(p.LastFrom);
        }

        if (p.NextCell) {
            glm::vec3 diff = p.NextCell->ToWorld() - c.ToWorld();
            float     dist = glm::length(diff);
            glm::vec3 dir  = glm::normalize(diff);
            spdlog::trace(
                "pos = {}({}), next = {}({}), diff = {}",
                c,
                t.Position,
                *p.NextCell,
                p.NextCell->ToWorld(),
                diff);

            if (dist < 1e-3f) {
                spdlog::trace("rounding pos to next cell");
                t.Position = p.NextCell->ToWorld();
            } else {
                auto force          = rb.Params.Velocity * dir;
                rb.Params.Direction = dir;
                spdlog::trace("advancing by {}", glm::to_string(force));
                // rigidBody.Body->setLinearVelocity(ToRP3D(dir));
                t.Position += force;
            }

            rb.Body->setTransform(t.ToRP3D());
            spdlog::trace("new pos: {}({})", GraphCell::FromWorldPoint(t.Position), t.Position);
        }
    }
}
