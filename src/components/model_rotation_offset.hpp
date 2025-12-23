#pragma once

#include <glm/gtc/quaternion.hpp>

// Rotation offset to apply to models that have different forward axes
// This offset is applied after calculating orientation from movement direction
struct ModelRotationOffset {
    glm::quat Offset{glm::identity<glm::quat>()};
};
