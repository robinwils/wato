#include <assimp/material.h>
#include <assimp/postprocess.h>  // Post processing flags
#include <assimp/scene.h>        // Output data structure

#include <entt/core/hashed_string.hpp>
#include <glm/gtx/string_cast.hpp>
#include <stdexcept>
#include <vector>

#include "core/cache.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "renderer/material.hpp"
#include "renderer/mesh_primitive.hpp"
#include "renderer/primitive.hpp"

using namespace entt::literals;

std::vector<entt::hashed_string> processMaterialTextures(const aiMaterial *material,
    aiTextureType                                                          type,
    aiString                                                              *path)
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

        auto textures = processMaterialTextures(material, aiTextureType_DIFFUSE, &diffuse_path);
        auto spec_textures =
            processMaterialTextures(material, aiTextureType_SPECULAR, &specular_path);

        if (textures.size() > 0 || spec_textures.size() > 0) {
            DBG("mesh %s has %d material textures", mesh->mName.C_Str(), textures.size());
            textures.reserve(textures.size() + spec_textures.size());
            textures.insert(textures.end(), spec_textures.begin(), spec_textures.end());
            throw std::runtime_error("not implemented");
        } else {
            // no material textures, get material info via properties
            aiColor3D diffuse;
            if (material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse) != AI_SUCCESS) {
                DBG("failed to get diffuse color for mesh %s", mesh->mName.C_Str());
            }
            aiColor3D specular;
            if (material->Get(AI_MATKEY_COLOR_SPECULAR, specular) != AI_SUCCESS) {
                DBG("failed to get specular color for mesh %s", mesh->mName.C_Str());
            }
            aiColor3D shininess;
            if (material->Get(AI_MATKEY_SHININESS, specular) != AI_SUCCESS) {
                DBG("failed to get specular color for mesh %s", mesh->mName.C_Str());
            }

            DBG("creating mesh with %d vertices and %d indices", vertices.size(), indices.size());
            auto     program = PROGRAM_CACHE["blinnphong"_hs];
            Material m(program,
                glm::vec3(diffuse.r, diffuse.g, diffuse.b),
                glm::vec3(specular.r, specular.g, specular.b));
            mp = new MeshPrimitive(std::move(vertices), std::move(indices), m);
        }
    } else {
        DBG("no material in mesh %s", mesh->mName.C_Str());
        throw std::runtime_error("no material in mesh");
    }
    return mp;
}

void processMetaData(const aiNode *node, const aiScene *scene)
{
    if (!node->mMetaData) {
        DBG("node %s metadata is null", node->mName.C_Str());
        return;
    }
    DBG("node %s has %d metadata", node->mName.C_Str(), node->mMetaData->mNumProperties);
    auto *mdata = node->mMetaData;
    for (unsigned int prop_idx = 0; prop_idx < mdata->mNumProperties; ++prop_idx) {
        auto &key = mdata->mKeys[prop_idx];
        auto &val = mdata->mValues[prop_idx];

        switch (val.mType) {
            case AI_BOOL:
                DBG("%s: bool", key.C_Str());
                break;
            case AI_INT32:
                DBG("%s: int32", key.C_Str());
                break;
            case AI_UINT64:
                DBG("%s: uint64", key.C_Str());
                break;
            case AI_FLOAT:
                DBG("%s: float", key.C_Str());
                break;
            case AI_DOUBLE:
                DBG("%s: double", key.C_Str());
                break;
            case AI_AISTRING:
                DBG("%s: aistring", key.C_Str());
                break;
            case AI_AIVECTOR3D:
                DBG("%s: vector3", key.C_Str());
                break;
            case AI_AIMETADATA:
                DBG("%s: mdata", key.C_Str());
                break;
            case AI_INT64:
                DBG("%s: int64", key.C_Str());
                break;
            case AI_UINT32:
                DBG("%s: uint32", key.C_Str());
                break;
            case AI_META_MAX:
                DBG("%s: meta max", key.C_Str());
                break;
            case FORCE_32BIT:
                DBG("%s: force 32bit", key.C_Str());
                break;
        }
    }
}

std::vector<Primitive *> processNode(const aiNode *node, const aiScene *scene)
{
    auto t         = node->mTransformation;
    auto transform = glm::identity<glm::mat4>();
    if (!node->mTransformation.IsIdentity()) {
        transform = glm::mat4(t.a1,
            t.a2,
            t.a3,
            t.a4,
            t.b1,
            t.b2,
            t.b3,
            t.b4,
            t.c1,
            t.c2,
            t.c3,
            t.c4,
            t.d1,
            t.d2,
            t.d3,
            t.d4);
        DBG("node %s has transformation %s",
            node->mName.C_Str(),
            glm::to_string(transform).c_str());
    } else {
        DBG("node %s has identity transform", node->mName.C_Str());
    }

    processMetaData(node, scene);
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
