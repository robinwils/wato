#pragma once

#include "glm/ext/vector_float3.hpp"
#include "glm/geometric.hpp"

struct Ray {
    Ray(glm::vec3 aOrigin, glm::vec3 aDir) : Orig(aOrigin), Dir(aDir) {}
    glm::vec3 Orig;
    glm::vec3 Dir;

    float IntersectPlane(const glm::vec3& aPlaneNormal) const
    {
        return glm::dot(-Orig, aPlaneNormal) / glm::dot(Dir, aPlaneNormal);
    }
};
