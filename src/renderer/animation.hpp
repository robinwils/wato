#pragma once

#include <spdlog/fmt/bundled/format.h>

#include <glm/ext/quaternion_common.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <string>
#include <unordered_map>
#include <vector>

template <typename KT>
struct AnimationKeyFrame {
    using key_type = KT;
    key_type Key;
    double   Time;

    constexpr key_type Interpolate(
        const AnimationKeyFrame<key_type>& aOther,
        const double                       aAnimTime) const
    {
        double f = (aAnimTime - Time) / (aOther.Time - Time);

        if constexpr (std::is_same_v<key_type, glm::vec3>) {
            return glm::mix(Key, aOther.Key, static_cast<float>(f));
        } else {
            return glm::slerp(Key, aOther.Key, static_cast<float>(f));
        }
    }
};

template <typename KT>
struct fmt::formatter<AnimationKeyFrame<KT>> : fmt::formatter<std::string> {
    auto format(AnimationKeyFrame<KT> aObj, format_context& aCtx) const -> decltype(aCtx.out())
    {
        return fmt::format_to(aCtx.out(), "{} at time {}", glm::to_string(aObj.Key), aObj.Time);
    }
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

    constexpr auto       Duration() { return mDuration; }
    constexpr auto       TicksPerSecond() { return mTicksPerSecond; }
    const NodeAnimation& GetNodeAnimation(const std::string& aBoneName) const
    {
        return mNodeAnimations.at(aBoneName);
    }

   private:
    std::string        mName;
    double             mDuration;
    double             mTicksPerSecond;
    node_animation_map mNodeAnimations;
};
