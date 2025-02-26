#include "core/ray.hpp"

#include <glm/ext/vector_float4.hpp>
#include <glm/matrix.hpp>

glm::vec4 Ray::word_cast(float w, float h)
{
    // viewport -> NDC
    float x_ndc = 2 * point.x / w - 1;
    float y_ndc = 2 * point.y / h - 1;

    // NDC -> view
    const auto& inv_proj = glm::inverse(camera.proj(w, h));
    glm::vec4   ray_ndc(x_ndc, y_ndc, 1.0f, 1.0f);
    glm::vec4   ray_view = inv_proj * ray_ndc;

    // view -> world
    const auto& inv_view = glm::inverse(camera.view(orig));

    return glm::normalize(inv_view * ray_view);
}
