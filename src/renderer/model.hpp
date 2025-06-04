#pragma once

#include <unordered_map>

#include "renderer/animation.hpp"
#include "renderer/primitive.hpp"
#include "renderer/skeleton.hpp"

class Model final
{
   public:
    using mesh_type      = PrimitiveVariant;
    using mesh_container = std::vector<mesh_type>;
    using animation_map  = std::unordered_map<std::string, Animation>;

    Model(mesh_container aMeshes, animation_map aAnimations)
        : mMeshes(std::move(aMeshes)), mAnimations(std::move(aAnimations))
    {
    }

    Model(mesh_container aMeshes, animation_map aAnimations, Skeleton aSkeleton)
        : mMeshes(std::move(aMeshes)),
          mAnimations(std::move(aAnimations)),
          mSkeleton(std::move(aSkeleton))
    {
    }

    void Submit(glm::mat4 aModelMatrix, uint64_t aState = BGFX_STATE_DEFAULT);

    const Skeleton&                Skeleton() const { return mSkeleton; }
   private:
    mesh_container mMeshes;
    animation_map  mAnimations;
    ::Skeleton     mSkeleton;  // bind pose
};
