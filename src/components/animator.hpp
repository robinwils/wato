#pragma once

#include <glm/glm.hpp>
#include <vector>

struct Animator {
    float                  Time;
    std::vector<glm::mat4> FinalBonesMatrices;
};
