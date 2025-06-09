#pragma once

#include <optional>
#include <string>
#include <vector>

#include "renderer/animation.hpp"

struct Animator {
    double                     Time;
    std::string                AnimationName;
    std::optional<::Animation> Animation;
    std::vector<glm::mat4>     FinalBonesMatrices;
};
