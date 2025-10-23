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
#include "core/graph.hpp"
#include "core/sys/log.hpp"

void AiSystem::operator()(Registry& aRegistry)
{
    auto& graph = aRegistry.ctx().get<Graph>();

    for (auto&& [e, creep, t, rb, p] :
         aRegistry.view<Creep, Transform3D, RigidBody, Path>().each()) {
        auto c = GraphCell::FromWorldPoint(t.Position);
        if (!p.NextCell || c == p.NextCell) {
            p.NextCell = graph.GetNextCell(c);
            p.LastFrom = c;
            WATO_TRACE(aRegistry, "set next cell = {} and last from = {}", p.NextCell, p.LastFrom);
        } else if (p.NextCell != graph.GetNextCell(p.LastFrom)) {
            // the path has probably been updated (tower built)
            WATO_TRACE(aRegistry, "path updated, setting next cell = ", graph.GetNextCell(p.LastFrom));
            p.NextCell = graph.GetNextCell(p.LastFrom);
        }

        aRegistry.patch<RigidBody>(e, [&p, &c](RigidBody& aBody) {
            if (p.NextCell) {
                aBody.Params.Direction = glm::normalize(p.NextCell->ToWorld() - c.ToWorld());
            } else {
                aBody.Params.Velocity = 0.0f;
            }
        });
    }
}
