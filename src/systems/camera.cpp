#include "components/camera.hpp"

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "components/transform3d.hpp"
#include "core/registry.hpp"

void cameraSystem(Registry& registry, float width, float height)
{
    for (auto&& [entity, camera, transform] : registry.view<Camera, Transform3D>().each()) {
        const auto& viewMat = camera.view(transform.position);
        const auto& proj    = camera.proj(width, height);
        bgfx::setViewTransform(0, glm::value_ptr(viewMat), glm::value_ptr(proj));
        bgfx::setViewRect(0, 0, 0, uint16_t(width), uint16_t(height));

        // just because I know there is only 1 camera (for now)
        break;
    }
}
