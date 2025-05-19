#pragma once

#include <assimp/postprocess.h>  // Post processing flags
#include <assimp/scene.h>        // Output data structure

#include <assimp/Importer.hpp>  // C++ importer interface

#include "core/sys/log.hpp"
#include "renderer/primitive.hpp"

template <>
struct fmt::formatter<aiString> : fmt::formatter<std::string> {
    auto format(aiString aObj, format_context& aCtx) const -> decltype(aCtx.out())
    {
        return fmt::format_to(aCtx.out(), "{}", aObj.C_Str());
    }
};

std::vector<Primitive<PositionNormalUvVertex>*> processNode(
    const aiNode*  aNode,
    const aiScene* aScene);

struct ModelLoader final {
    using result_type = std::shared_ptr<std::vector<Primitive<PositionNormalUvVertex>*>>;

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
            return std::make_shared<std::vector<Primitive<PositionNormalUvVertex>*>>();
        }
        DBG("scene {} has:", scene->mName);
        DBG("  {} meshes", scene->mNumMeshes);
        DBG("  {} textures", scene->mNumTextures);
        DBG("  {} materials", scene->mNumMaterials);
        DBG("  {} animations", scene->mNumAnimations);

        auto meshes = processNode(scene->mRootNode, scene);

        return std::make_shared<std::vector<Primitive<PositionNormalUvVertex>*>>(meshes);
    }

    template <typename... Args>
    result_type operator()(Primitive<PositionNormalUvVertex>* aPrimitive)
    {
        auto meshes = std::vector({aPrimitive});
        return std::make_shared<std::vector<Primitive<PositionNormalUvVertex>*>>(meshes);
    }
};
