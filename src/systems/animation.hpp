#pragma once

#include "components/animator.hpp"
#include "renderer/model.hpp"
#include "renderer/skeleton.hpp"
#include "systems/system.hpp"

/**
 * @brief Skeletal animation system (frame time)
 *
 * Interpolates skeletal animations for smooth rendering.
 * Runs at variable frame rate for smooth visual output.
 */
class AnimationSystem : public FrameSystem
{
   public:
    using FrameSystem::FrameSystem;

   protected:
    void Execute(Registry& aRegistry, float aDelta) override;

   private:
    struct AnimationContext {
        const ::Skeleton* Skeleton;
        ::Animator*       Animator;
        double            Time;
        glm::mat4         GlobalInverse;
    };

    void animateBone(
        const AnimationContext& aAnimCtx,
        const Bone::index_type  aBoneIdx,
        const glm::mat4&        aParentTransform);

    template <typename KT>
    KT interpolateKey(std::vector<AnimationKeyFrame<KT>> aKeys, const double aTime);
};
