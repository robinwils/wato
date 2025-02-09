#pragma once

#include <assimp/postprocess.h>  // Post processing flags
#include <assimp/scene.h>        // Output data structure

#include <assimp/Importer.hpp>  // C++ importer interface

#include "core/sys.hpp"
#include "renderer/material.hpp"
#include "renderer/mesh_primitive.hpp"

std::vector<MeshPrimitive> processNode(const aiNode *node, const aiScene *scene);

struct ModelLoader final {
    using result_type = std::vector<MeshPrimitive>;

    template <typename... Args>
    result_type operator()(const char *_name)
    {
        Assimp::Importer importer;

        // flags are used for post processing (the more, the slower)
        const aiScene *scene = importer.ReadFile(_name,
            aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_JoinIdenticalVertices
                | aiProcess_SortByPType);

        // If the import failed, report it
        if (nullptr == scene) {
            // TODO: handle error
            DBG("could not load %s", _name);
        }

        auto meshes = processNode(scene->mRootNode, scene);

        DBG("done loading model");
        return meshes;
    }
};
