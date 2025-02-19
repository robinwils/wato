#pragma once

#include <assimp/postprocess.h>  // Post processing flags
#include <assimp/scene.h>        // Output data structure

#include <assimp/Importer.hpp>  // C++ importer interface

#include "core/sys.hpp"
#include "renderer/primitive.hpp"

std::vector<Primitive *> processNode(const aiNode *node, const aiScene *scene);

struct ModelLoader final {
    using result_type = std::shared_ptr<std::vector<Primitive *>>;

    template <typename... Args>
    result_type operator()(const char *_name)
    {
        Assimp::Importer importer;

        // flags are used for post processing (the more, the slower)
        const aiScene *scene = importer.ReadFile(_name,
            aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_PreTransformVertices | aiProcess_GlobalScale);

        // If the import failed, report it
        if (nullptr == scene) {
            // TODO: handle error
            DBG("could not load %s", _name);
            return std::make_shared<std::vector<Primitive *>>();
        }
        DBG("scene %s has:", scene->mName);
        DBG("  %d meshes", scene->mNumMeshes);
        DBG("  %d textures", scene->mNumTextures);
        DBG("  %d materials", scene->mNumMaterials);
        DBG("  %d animations", scene->mNumAnimations);

        auto meshes = processNode(scene->mRootNode, scene);

        return std::make_shared<std::vector<Primitive *>>(meshes);
    }

    template <typename... Args>
    result_type operator()(Primitive *primitive)
    {
        auto meshes = std::vector({primitive});
        return std::make_shared<std::vector<Primitive *>>(meshes);
    }
};
