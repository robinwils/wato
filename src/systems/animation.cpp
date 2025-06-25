#include "systems/animation.hpp"

#include <spdlog/spdlog.h>

#include <glm/gtx/string_cast.hpp>
#include <stdexcept>

#include "components/scene_object.hpp"
#include "components/transform3d.hpp"
#include "registry/registry.hpp"
#include "renderer/cache.hpp"

void AnimationSystem::operator()(Registry& aRegistry, const float aDeltaTime)
{
    for (auto&& [entity, obj, animator] : aRegistry.view<SceneObject, Animator>().each()) {
        if (auto model = WATO_MODEL_CACHE[obj.ModelHash]; model) {
            if (!animator.Animation
                && !(animator.Animation = model->GetAnimation(animator.AnimationName))) {
                spdlog::warn("could not get animation {}, ignoring", animator.AnimationName);
                continue;
            }
            const Skeleton& skeleton = model->Skeleton();

            if (skeleton.Bones.empty()) {
                spdlog::warn("got empty skeleton for {}", obj.ModelHash.data());
                continue;
            }

            animator.Time += static_cast<double>(aDeltaTime);

            double animatonTimeTicks = animator.Time * animator.Animation->TicksPerSecond();
            double animationTime     = std::fmod(animatonTimeTicks, animator.Animation->Duration());

            if (animator.FinalBonesMatrices.empty()) {
                spdlog::debug("empty final bone vertices, reserving {}", skeleton.Bones.size());
                animator.FinalBonesMatrices.resize(skeleton.Bones.size());
            }

            animateBone(
                AnimationContext{
                    .Skeleton      = &skeleton,
                    .Animator      = &animator,
                    .Time          = animationTime,
                    .GlobalInverse = model->GlobalInverse(),
                },
                0,
                glm::identity<glm::mat4>());
        }
    }
}

template <typename KT>
KT AnimationSystem::interpolateKey(std::vector<AnimationKeyFrame<KT>> aKeys, const double aTime)
{
    if (aKeys.empty()) {
        throw std::runtime_error("got empty animation keys");
    }
    if (aTime <= aKeys.front().Time) {
        return aKeys.front().Key;
    }
    if (aTime >= aKeys.back().Time) {
        return aKeys.back().Key;
    }
    for (unsigned int posIdx = 0; posIdx < aKeys.size() - 1; ++posIdx) {
        if (aTime < aKeys[posIdx + 1].Time) {
            return aKeys[posIdx].Interpolate(aKeys[posIdx + 1], aTime);
        }
    }
    return aKeys.back().Key;
}

void AnimationSystem::animateBone(
    const AnimationContext& aAnimCtx,
    const Bone::index_type  aBoneIdx,
    const glm::mat4&        aParentTransform)
{
    const Bone&          bone     = aAnimCtx.Skeleton->Bones[aBoneIdx];
    const NodeAnimation& nodeAnim = aAnimCtx.Animator->Animation->GetNodeAnimation(bone.Name);

    Transform3D transform{
        .Position    = interpolateKey(nodeAnim.Positions, aAnimCtx.Time),
        .Orientation = interpolateKey(nodeAnim.Rotations, aAnimCtx.Time),
        .Scale       = interpolateKey(nodeAnim.Scales, aAnimCtx.Time),
    };
    glm::mat4 global = aParentTransform * transform.ModelMat();

    spdlog::trace("got global mat {}", glm::to_string(global));

    aAnimCtx.Animator->FinalBonesMatrices[aBoneIdx] =
        aAnimCtx.GlobalInverse * global * bone.Offset.value();

    for (const Bone::index_type cIdx : bone.Children) {
        animateBone(aAnimCtx, cIdx, global);
    }
}
