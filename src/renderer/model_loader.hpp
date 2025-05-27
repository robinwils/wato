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

class ModelLoader final
{
   public:
    using mesh_type      = Model::mesh_type;
    using mesh_container = Model::mesh_container;
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
        DBG("model {} has:", aName);
        DBG("  {} meshes", scene->mNumMeshes);
        DBG("  {} embedded textures", scene->mNumTextures);
        DBG("  {} materials", scene->mNumMaterials);
        DBG("  {} animations", scene->mNumAnimations);

        Skeleton skeleton;
        if (scene->HasAnimations()) {
            buildSkeleton(scene->mRootNode, scene, skeleton, 0);
        }
        auto          meshes = processNode(scene->mRootNode, scene, skeleton);
        animation_map animations;
        if (scene->HasAnimations()) {
            animations = processAnimations(scene);
        }

        PrintSkeleton(skeleton);

        return std::make_shared<Model>(std::move(meshes), std::move(animations));
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
    void          processBones(
                 const aiMesh*                            aMesh,
                 const aiScene*                           aScene,
                 std::vector<PositionNormalUvBoneVertex>& aVertices,
                 Skeleton&                                aSkeleton);
    std::size_t buildSkeleton(
        const aiNode*  aNode,
        const aiScene* aScene,
        Skeleton&      aSkeleton,
        std::size_t    aIndent);

    std::unordered_map<std::string, std::size_t> mBonesMap;
};
