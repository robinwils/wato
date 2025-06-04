#pragma once

#include <glm/ext/vector_float3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <unordered_map>
#include <vector>

struct PositionKey {
    glm::vec3 Position;
    double    Time;
};

struct QuatKey {
    glm::quat Rotation;
    double    Time;
};

struct ScalingKey {
    glm::vec3 Scaling;
    double    Time;
};

struct NodeAnimation {
    std::string Name;

    std::vector<AnimationKeyFrame<glm::vec3>> Positions;
    std::vector<AnimationKeyFrame<glm::quat>> Rotations;
    std::vector<AnimationKeyFrame<glm::vec3>> Scales;
};

class Animation
{
   public:
    using node_animation_map = std::unordered_map<std::string, NodeAnimation>;
    Animation(
        std::string        aName,
        double             aDuration,
        double             aTicksPerSecond,
        node_animation_map aNodeAnimations)
        : mName(std::move(aName)),
          mDuration(aDuration),
          mTicksPerSecond(aTicksPerSecond),
          mNodeAnimations(std::move(aNodeAnimations))
    {
    }
    Animation(const Animation&)            = default;
    Animation(Animation&&)                 = default;
    Animation& operator=(const Animation&) = default;
    Animation& operator=(Animation&&)      = default;
    virtual ~Animation()                   = default;

    constexpr auto Duration() { return mDuration; }
    constexpr auto TicksPerSecond() { return mTicksPerSecond; }

   private:
    std::string        mName;
    double             mDuration;
    double             mTicksPerSecond;
    node_animation_map mNodeAnimations;
};
