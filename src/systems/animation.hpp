#pragma once

#include "components/animator.hpp"
#include "renderer/model.hpp"
#include "renderer/skeleton.hpp"
#include "systems/system.hpp"

class AnimationSystem : public System<AnimationSystem>
{
   public:
    void operator()(Registry& aRegistry, const float aDeltaTime);

    static constexpr const char* StaticName() { return "AnimationSystem"; }

   private:
    struct AnimationContext {
        const Skeleton* Skeleton;
        Animator*       Animator;
        double          Time;
        glm::mat4       GlobalInverse;
    };

    void animateBone(
        const AnimationContext& aAnimCtx,
        const Bone::index_type  aBoneIdx,
        const glm::mat4&        aParentTransform);

    template <typename KT>
    KT interpolateKey(std::vector<AnimationKeyFrame<KT>> aKeys, const double aTime);
};
