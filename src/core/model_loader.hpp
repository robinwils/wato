#pragma once

#include <assimp/postprocess.h>  // Post processing flags
#include <assimp/scene.h>        // Output data structure

#include <assimp/Importer.hpp>  // C++ importer interface

#include "core/sys.hpp"
#include "renderer/primitive.hpp"

std::vector<Primitive<PositionNormalUvVertex> *> processNode(const aiNode *aNode,
    const aiScene                                                         *aScene);

struct ModelLoader final {
    using result_type = std::shared_ptr<std::vector<Primitive<PositionNormalUvVertex> *>>;

    template <typename... Args>
    result_type operator()(const char *aName, unsigned int aPostProcessFlags)
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

        const aiScene *scene = importer.ReadFile(aName, aPostProcessFlags);

        // If the import failed, report it
        if (nullptr == scene) {
            // TODO: handle error
            DBG("could not load %s", aName);
            return std::make_shared<std::vector<Primitive<PositionNormalUvVertex> *>>();
        }
        DBG("scene %s has:", scene->mName.C_Str());
        DBG("  %d meshes", scene->mNumMeshes);
        DBG("  %d textures", scene->mNumTextures);
        DBG("  %d materials", scene->mNumMaterials);
        DBG("  %d animations", scene->mNumAnimations);

        auto meshes = processNode(scene->mRootNode, scene);

        return std::make_shared<std::vector<Primitive<PositionNormalUvVertex> *>>(meshes);
    }

    template <typename... Args>
    result_type operator()(Primitive<PositionNormalUvVertex> *aPrimitive)
    {
        auto meshes = std::vector({aPrimitive});
        return std::make_shared<std::vector<Primitive<PositionNormalUvVertex> *>>(meshes);
    }
};
