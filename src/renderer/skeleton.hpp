#pragma once

#include <glm/ext/matrix_float4x4.hpp>
#include <string>
#include <vector>

struct Bone {
    std::string      Name;
    glm::mat4        Offset;
    std::vector<int> Children;
};

struct Skeleton {
    std::vector<Bone> Bones;
};
