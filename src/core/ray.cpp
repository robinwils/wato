#include "components/camera.hpp"
#include "glm/ext/vector_float4.hpp"

glm::vec4 ray_cast(const Camera& cam, float w, float h, glm::dvec2 point)
{
    // view -> NDC
    float x_ndc = 2 * point.x / w - 1;
    float y_ndc = 2 * point.y / h - 1;

    // NDC -> view
    const auto& inv_proj = glm::inverse(cam.proj(w, h));
    glm::vec4   ray_ndc(x_ndc, y_ndc, 1.0f, 1.0f);
    glm::vec4   ray_view = inv_proj * ray_ndc;

    return ray_view;
}
