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

using namespace entt::literals;

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

    MeshPrimitive *mp;
    if (mesh->mMaterialIndex >= 0) {
        auto        diffuse_path  = aiString("texture_diffuse");
        auto        specular_path = aiString("texture_specular");
        const auto *material      = scene->mMaterials[mesh->mMaterialIndex];

        auto textures      = processMaterialTextures(material, aiTextureType_DIFFUSE, &diffuse_path);
        auto spec_textures = processMaterialTextures(material, aiTextureType_SPECULAR, &specular_path);

        if (textures.size() > 0 || spec_textures.size() > 0) {
            DBG("mesh %s has %d material textures", mesh->mName.C_Str(), textures.size());
            textures.reserve(textures.size() + spec_textures.size());
            textures.insert(textures.end(), spec_textures.begin(), spec_textures.end());
        } else {
            // no material textures, get material info via properties
            DBG("parsing material properties for mesh %s", mesh->mName.C_Str());
            for (unsigned int pIdx = 0; pIdx < material->mNumProperties; ++pIdx) {
                auto *matProp = material->mProperties[pIdx];
                DBG("  %s: len=%d type=%d", matProp->mKey.C_Str(), matProp->mDataLength, matProp->mType);
            }
            aiColor3D diffuse;
            if (material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse) != AI_SUCCESS) {
                DBG("failed to get diffuse color for mesh %s", mesh->mName.C_Str());
            }
            aiColor3D specular;
            if (material->Get(AI_MATKEY_COLOR_SPECULAR, specular) != AI_SUCCESS) {
                DBG("failed to get specular color for mesh %s", mesh->mName.C_Str());
            }

            auto     program = PROGRAM_CACHE["blinnphong"_hs];
            Material m(program,
                glm::vec3(diffuse.r, diffuse.g, diffuse.b),
                glm::vec3(specular.r, specular.g, specular.b));
            mp = new MeshPrimitive(std::move(vertices), std::move(indices), m);
        }
        DBG("creating mesh with %d vertices and %d indices", vertices.size(), indices.size());
    } else {
        DBG("no material in mesh %s", mesh->mName.C_Str());
        throw std::runtime_error("no material in mesh");
    }
    return mp;
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
