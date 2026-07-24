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

template <typename VecDst, typename VecSrc>
VecDst ToVec3(const VecSrc& aPt)
{
    return {aPt.x, aPt.y, aPt.z};
}

struct Frustum {
    static constexpr size_t kNumPlanes = 6;

    // using planes_type = std::array<glm::vec4, kNumPlanes>;
    using planes_type = std::array<bx::Plane, kNumPlanes>;
    planes_type Planes;

    [[nodiscard]] bool Contains(const glm::vec3& aPoint) const
    {
        auto point = ToVec3<bx::Vec3>(aPoint);
        return std::ranges::all_of(Planes, [&](const bx::Plane& aPlane) {
            return bx::overlap(aPlane, point);
        });
    }

    [[nodiscard]] bool Intersects(const glm::vec3& aMin, const glm::vec3& aMax) const
    {
        bx::Aabb aabb{.min = ToVec3<bx::Vec3>(aMin), .max = ToVec3<bx::Vec3>(aMax)};
        return std::ranges::all_of(Planes, [&](const bx::Plane& aPlane) {
            return bx::overlap(aPlane, aabb);
        });
    }

    static Frustum MakeCameraFrustum(const glm::mat4& aMat, bool aHomogeneousDepth)
    {
        planes_type planes{
            bx::InitNone,
            bx::InitNone,
            bx::InitNone,
            bx::InitNone,
            bx::InitNone,
            bx::InitNone,
        };
        bx::buildFrustumPlanes(planes.data(), glm::value_ptr(aMat), aHomogeneousDepth);
        // planes_type planes{
        //     row(aMat, 3) + row(aMat, 0),                       // left
        //     row(aMat, 3) - row(aMat, 0),                       // right
        //     row(aMat, 3) + row(aMat, 1),                       // bottom
        //     row(aMat, 3) - row(aMat, 1),                       // top
        //     aHomogeneousDepth ? (row(aMat, 3) + row(aMat, 2))  // near, GL  [-1,1]
        //                       : row(aMat, 2),                  // near, D3D [0,1]
        //     row(aMat, 3) - row(aMat, 2),                       // far
        // };
        //
        // for (auto& plane : planes) {
        //     plane /= glm::length(glm::vec3(plane));
        // }
        return Frustum{.Planes = planes};
    }
};
