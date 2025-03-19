#include "components/camera.hpp"

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "components/transform3d.hpp"
#include "core/registry.hpp"

void cameraSystem(Registry& aRegistry, float aWidth, float aHeight)
{
    for (auto&& [entity, camera, transform] : aRegistry.view<Camera, Transform3D>().each()) {
        const auto& viewMat = camera.View(transform.Position);
        const auto& proj    = camera.Projection(aWidth, aHeight);
        bgfx::setViewTransform(0, glm::value_ptr(viewMat), glm::value_ptr(proj));
        bgfx::setViewRect(0, 0, 0, uint16_t(aWidth), uint16_t(aHeight));

        // just because I know there is only 1 camera (for now)
        break;
    }
}
