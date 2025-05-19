#pragma once

#include <assimp/postprocess.h>  // Post processing flags
#include <assimp/scene.h>        // Output data structure

#include <assimp/Importer.hpp>  // C++ importer interface
#include <entt/core/hashed_string.hpp>

#include "core/sys/log.hpp"
#include "renderer/primitive.hpp"

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
    using mesh_type      = Primitive<PositionNormalUvVertex>;
    using mesh_container = std::vector<mesh_type*>;
    using result_type    = std::shared_ptr<mesh_container>;

    template <typename... Args>
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

        const aiScene* scene = importer.ReadFile(aName, aPostProcessFlags);

        // If the import failed, report it
        if (nullptr == scene) {
            // TODO: handle error
            DBG("could not load {}", aName);
            return std::make_shared<mesh_container>();
        }
        DBG("scene {} has:", scene->mName);
        DBG("  {} meshes", scene->mNumMeshes);
        DBG("  {} textures", scene->mNumTextures);
        DBG("  {} materials", scene->mNumMaterials);
        DBG("  {} animations", scene->mNumAnimations);

        auto meshes = processNode(scene->mRootNode, scene);

        return std::make_shared<mesh_container>(meshes);
    }

    template <typename... Args>
    result_type operator()(mesh_type* aPrimitive)
    {
        auto meshes = std::vector({aPrimitive});
        return std::make_shared<mesh_container>(meshes);
    }

   private:
    std::vector<entt::hashed_string>
    processMaterialTextures(const aiMaterial* aMaterial, aiTextureType aType, aiString* aPath);

    mesh_container processNode(const aiNode* aNode, const aiScene* aScene);
    mesh_type*     processMesh(const aiMesh* aMesh, const aiScene* aScene);
    void           processMetaData(const aiNode* aNode, const aiScene* /*aScene*/);
};
