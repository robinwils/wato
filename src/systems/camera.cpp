#include "components/camera.hpp"

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "components/transform3d.hpp"
#include "core/registry.hpp"

void cameraSystem(Registry& registry, float width, float height)
{
    for (auto&& [entity, camera, transform] : registry.view<Camera, Transform3D>().each()) {
        const auto& viewMat = glm::lookAt(transform.position, transform.position + camera.dir, camera.up);
        const auto& proj =
            glm::perspective(glm::radians(camera.fov), width / height, camera.near_clip, camera.far_clip);
        bgfx::setViewTransform(0, glm::value_ptr(viewMat), glm::value_ptr(proj));
        bgfx::setViewRect(0, 0, 0, uint16_t(width), uint16_t(height));

        // just because I know there is only 1 camera (for now)
        break;
    }
}
