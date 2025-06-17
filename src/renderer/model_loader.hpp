#pragma once

#include <assimp/postprocess.h>  // Post processing flags
#include <assimp/scene.h>        // Output data structure

#include <assimp/Importer.hpp>  // C++ importer interface
#include <entt/core/hashed_string.hpp>
#include <optional>
#include <stdexcept>

#include "core/sys/log.hpp"
#include "renderer/animation.hpp"
#include "renderer/asset.hpp"
#include "renderer/model.hpp"
#include "renderer/skeleton.hpp"
#include "renderer/vertex_layout.hpp"

template <>
struct fmt::formatter<aiString> : fmt::formatter<std::string> {
    auto format(aiString aObj, format_context& aCtx) const -> decltype(aCtx.out())
    {
        return fmt::format_to(aCtx.out(), "{}", aObj.C_Str());
    }
};

constexpr inline glm::vec3 toGLMVec3(const aiVector3D& aVector)
{
    return glm::vec3(aVector.x, aVector.y, aVector.z);
}

constexpr inline glm::mat4 toGLMMat4(const aiMatrix4x4& aMat)
{
    // clang-format off
    return glm::mat4(
        aMat.a1, aMat.b1, aMat.c1, aMat.d1,
        aMat.a2, aMat.b2, aMat.c2, aMat.d2,
        aMat.a3, aMat.b3, aMat.c3, aMat.d3,
        aMat.a4, aMat.b4, aMat.c4, aMat.d4
    );
    // clang-format on
}

class ModelLoader final
{
   public:
    using mesh_type      = Model::mesh_type;
    using mesh_container = Model::mesh_container;
    using bone_index_map = std::unordered_map<std::string, std::optional<std::size_t>>;
    using animation_map  = Model::animation_map;
    using result_type    = std::shared_ptr<Model>;

    result_type operator()(const char* aName, unsigned int aPostProcessFlags)
    {
        Assimp::Importer importer;

        // flags are used for post processing (the more, the slower)
        // const aiScene *scene = importer.ReadFile(_name,
        //     aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_PreTransformVertices |
        //     aiProcess_GlobalScale);
        //
        // const aiScene *scene = importer.ReadFile(_name,
        //     aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs |
        //     aiProcess_JoinIdenticalVertices);

        std::string assetPath = FindAsset(aName);
        if (assetPath == "") {
            throw std::runtime_error(fmt::format("cannot find asset {}", aName));
        }

        const aiScene* scene = importer.ReadFile(assetPath, aPostProcessFlags);

        // If the import failed, report it
        if (nullptr == scene) {
            throw std::runtime_error(
                fmt::format("could not load {}: {}", assetPath, importer.GetErrorString()));
        }
        spdlog::debug("model {} has:", aName);
        spdlog::debug("  {} meshes", scene->mNumMeshes);
        spdlog::debug("  {} embedded textures", scene->mNumTextures);
        spdlog::debug("  {} materials", scene->mNumMaterials);
        spdlog::debug("  {} animations", scene->mNumAnimations);

        Skeleton skeleton;
        if (scene->HasAnimations()) {
            populateBoneNames(scene->mRootNode, scene);
            buildSkeleton(scene->mRootNode, scene, skeleton, 0);
            spdlog::info("skeleton with {} bones built", skeleton.Bones.size());
            if (!mBonesMap.empty()) {
                for (std::size_t i = 0; i < skeleton.Bones.size(); ++i) {
                    spdlog::debug("Bone[{}]: {}", i, skeleton.Bones[i]);
                }
            } else {
                spdlog::warn("got animations but no bones");
            }
        }
        auto          meshes = processNode(scene->mRootNode, scene, skeleton);
        animation_map animations;
        if (scene->HasAnimations()) {
            animations = processAnimations(scene);
        }

        PrintSkeleton(skeleton);

        return std::make_shared<Model>(
            std::move(meshes),
            std::move(animations),
            std::move(skeleton),
            toGLMMat4(scene->mRootNode->mTransformation));
    }

    result_type operator()(mesh_type aPrimitive)
    {
        mesh_container meshes;
        meshes.push_back(std::move(aPrimitive));
        return std::make_shared<Model>(meshes, animation_map{});
    }

   private:
    std::vector<entt::hashed_string>
    processMaterialTextures(const aiMaterial* aMaterial, aiTextureType aType, aiString* aPath);

    mesh_container processNode(const aiNode* aNode, const aiScene* aScene, Skeleton& aSkeleton);

    template <typename VL>
    mesh_type     processMesh(const aiMesh* aMesh, const aiScene* aScene, Skeleton& aSkeleton);
    void          processMetaData(const aiNode* aNode, const aiScene* /*aScene*/);
    animation_map processAnimations(const aiScene* aScene);
    Animation     processAnimation(const aiAnimation* aAnimation);
    void processBones(const aiMesh* aMesh, std::vector<PositionNormalUvBoneVertex>& aVertices);
    void populateBoneNames(const aiNode* aNode, const aiScene* aScene);
    std::size_t buildSkeleton(
        const aiNode*  aNode,
        const aiScene* aScene,
        Skeleton&      aSkeleton,
        std::size_t    aIndent);

    bone_index_map mBonesMap;
};
