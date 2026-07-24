#pragma once

#include <bx/bounds.h>

#include <algorithm>
#include <array>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

inline glm::vec4 row(const glm::mat4& aMat, size_t aIdx)
{
    return {aMat[0][aIdx], aMat[1][aIdx], aMat[2][aIdx], aMat[3][aIdx]};
};

struct Frustum {
    static constexpr size_t kNumPlanes = 6;

    using planes_type = std::array<glm::vec4, kNumPlanes>;
    planes_type Planes;

    [[nodiscard]] bool Contains(const glm::vec3& aPoint) const
    {
        return std::ranges::all_of(Planes, [&](const glm::vec4& aPlane) {
            return (glm::dot(glm::vec3(aPlane), aPoint) + aPlane.w >= 0);
        });
    }

    [[nodiscard]] bool Intersects(const glm::vec3& aMin, const glm::vec3& aMax) const
    {
        return std::ranges::all_of(Planes, [&](const glm::vec4& aPlane) {
            glm::vec3 normal{aPlane};
            glm::vec3 closest{
                normal.x >= 0 ? aMax.x : aMin.x,
                normal.y >= 0 ? aMax.y : aMin.y,
                normal.z >= 0 ? aMax.z : aMin.z,
            };
            return glm::dot(normal, closest) + aPlane.w >= 0;
        });
    }

    static Frustum MakeFrustum(const glm::mat4& aMat, bool aHomogeneousDepth)
    {
        planes_type planes{
            row(aMat, 3) + row(aMat, 0),                       // left
            row(aMat, 3) - row(aMat, 0),                       // right
            row(aMat, 3) + row(aMat, 1),                       // bottom
            row(aMat, 3) - row(aMat, 1),                       // top
            aHomogeneousDepth ? (row(aMat, 3) + row(aMat, 2))  // near, GL  [-1,1]
                              : row(aMat, 2),                  // near, D3D [0,1]
            row(aMat, 3) - row(aMat, 2),                       // far
        };

        for (auto& plane : planes) {
            plane /= glm::length(glm::vec3(plane));
        }
        return Frustum{.Planes = planes};
    }
};
