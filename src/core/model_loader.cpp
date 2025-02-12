#include <assimp/material.h>
#include <assimp/postprocess.h>  // Post processing flags
#include <assimp/scene.h>        // Output data structure

#include <entt/core/hashed_string.hpp>
#include <stdexcept>
#include <vector>

#include "core/cache.hpp"
#include "renderer/material.hpp"
#include "renderer/mesh_primitive.hpp"
#include "renderer/primitive.hpp"

std::vector<entt::hashed_string> processMaterialTextures(const aiMaterial *material, aiTextureType type, aiString *path)
{
    std::vector<entt::hashed_string> textures;
    for (unsigned int i = 0; i < material->GetTextureCount(type); ++i) {
        auto res = material->GetTexture(type, i, path);
        if (AI_SUCCESS != res) {
            throw std::runtime_error("could not get texture");
        }
        auto hs           = entt::hashed_string{path->C_Str()};
        auto [it, loaded] = TEXTURE_CACHE.load(hs, path->C_Str());
        auto diffuse      = TEXTURE_CACHE[hs];

        if (!bgfx::isValid(diffuse)) {
            throw std::runtime_error("could not load model material texture, invalid handle");
        }

        textures.push_back(hs);
    }

    return textures;
}

Primitive *processMesh(const aiMesh *mesh, const aiScene *scene)
{
    std::vector<PositionNormalUvVertex> vertices;
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
        PositionNormalUvVertex vertex;
        vertex.position.x = mesh->mVertices[i].x;
        vertex.position.y = mesh->mVertices[i].y;
        vertex.position.z = mesh->mVertices[i].z;

        vertex.normal.x = mesh->mNormals[i].x;
        vertex.normal.y = mesh->mNormals[i].y;
        vertex.normal.z = mesh->mNormals[i].z;

        if (mesh->mTextureCoords[0]) {
            vertex.uv.x = mesh->mTextureCoords[0][i].x;
            vertex.uv.y = mesh->mTextureCoords[0][i].y;
        }
        vertices.push_back(vertex);
    }

    std::vector<uint16_t> indices;
    for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
        auto face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j) {
            indices.push_back(face.mIndices[j]);
        }
    }

    if (mesh->mMaterialIndex >= 0) {
        auto        diffuse_path  = aiString("texture_diffuse");
        auto        specular_path = aiString("texture_specular");
        const auto *material      = scene->mMaterials[mesh->mMaterialIndex];

        auto textures      = processMaterialTextures(material, aiTextureType_DIFFUSE, &diffuse_path);
        auto spec_textures = processMaterialTextures(material, aiTextureType_SPECULAR, &specular_path);

        textures.reserve(textures.size() + spec_textures.size());
        textures.insert(textures.end(), spec_textures.begin(), spec_textures.end());

        // auto [it, loaded] = TEXTURE_CACHE.load(hs, path->C_Str());
        // auto diffuse      = TEXTURE_CACHE[hs];
        //
        // if (!bgfx::isValid(diffuse)) {
        //     throw std::runtime_error("could not load model material texture, invalid handle");
        // }
        //
        // auto program = PROGRAM_CACHE["blinnphong"_hs];
        //
        // Material material(program, diffuse, specular);
    }
    DBG("creating mesh with %d vertices and %d indices", vertices.size(), indices.size());

    return new MeshPrimitive(std::move(vertices), std::move(indices));
}

std::vector<Primitive *> processNode(const aiNode *node, const aiScene *scene)
{
    DBG("processing node")
    std::vector<Primitive *> meshes;
    for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
        auto *mesh = processMesh(scene->mMeshes[node->mMeshes[i]], scene);
        meshes.push_back(mesh);
    }
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        auto child_meshes = processNode(node->mChildren[i], scene);
        meshes.insert(meshes.end(), child_meshes.begin(), child_meshes.end());
    }
    return meshes;
}
