#include <assimp/material.h>
#include <assimp/postprocess.h>  // Post processing flags
#include <assimp/scene.h>        // Output data structure

#include <glm/gtx/string_cast.hpp>
#include <stdexcept>
#include <vector>

#include "core/cache.hpp"
#include "core/sys/mem.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "renderer/blinn_phong_material.hpp"
#include "renderer/mesh_primitive.hpp"
#include "renderer/primitive.hpp"

using namespace entt::literals;

std::vector<entt::hashed_string> ModelLoader::processMaterialTextures(
    const aiMaterial* aMaterial,
    aiTextureType     aType,
    aiString*         aPath)
{
    std::vector<entt::hashed_string> textures;
    for (unsigned int i = 0; i < aMaterial->GetTextureCount(aType); ++i) {
        aiReturn res = aMaterial->GetTexture(aType, i, aPath);
        if (AI_SUCCESS != res) {
            throw std::runtime_error("could not get texture");
        }

        auto hs          = entt::hashed_string{aPath->C_Str()};
        auto [_, loaded] = WATO_TEXTURE_CACHE.load(hs, aPath->C_Str());
        if (!loaded) {
            throw std::runtime_error("could not load model material texture");
        }

        auto diffuse = WATO_TEXTURE_CACHE[hs];
        if (!bgfx::isValid(diffuse)) {
            throw std::runtime_error("could not load model material texture, invalid handle");
        }

        textures.push_back(hs);
    }

    return textures;
}

ModelLoader::mesh_type* ModelLoader::processMesh(const aiMesh* aMesh, const aiScene* aScene)
{
    std::vector<PositionNormalUvVertex> vertices;
    for (unsigned int i = 0; i < aMesh->mNumVertices; ++i) {
        PositionNormalUvVertex vertex{};
        vertex.Position.x = aMesh->mVertices[i].x;
        vertex.Position.y = aMesh->mVertices[i].y;
        vertex.Position.z = aMesh->mVertices[i].z;

        vertex.Normal.x = aMesh->mNormals[i].x;
        vertex.Normal.y = aMesh->mNormals[i].y;
        vertex.Normal.z = aMesh->mNormals[i].z;

        if (aMesh->mTextureCoords[0]) {
            vertex.Uv.x = aMesh->mTextureCoords[0][i].x;
            vertex.Uv.y = aMesh->mTextureCoords[0][i].y;
        }
        vertices.push_back(vertex);
    }

    std::vector<MeshPrimitive::indice_type> indices;
    for (unsigned int i = 0; i < aMesh->mNumFaces; ++i) {
        auto face = aMesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j) {
            indices.push_back(face.mIndices[j]);
        }
    }

    MeshPrimitive* mp = nullptr;
    if (aMesh->mMaterialIndex >= 0) {
        auto        diffusePath  = aiString("texture_diffuse");
        auto        specularPath = aiString("texture_specular");
        const auto* material     = aScene->mMaterials[aMesh->mMaterialIndex];

        auto textures = processMaterialTextures(material, aiTextureType_DIFFUSE, &diffusePath);
        auto specTextures =
            processMaterialTextures(material, aiTextureType_SPECULAR, &specularPath);

        if (textures.size() > 0 || specTextures.size() > 0) {
            DBG("mesh {} has {} material textures", aMesh->mName, textures.size());
            textures.reserve(textures.size() + specTextures.size());
            textures.insert(textures.end(), specTextures.begin(), specTextures.end());
            throw std::runtime_error("not implemented");
        } else {
            // no material textures, get material info via properties
            aiColor3D diffuse;
            if (material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse) != AI_SUCCESS) {
                DBG("failed to get diffuse color for mesh {}", aMesh->mName);
            }
            aiColor3D specular;
            if (material->Get(AI_MATKEY_COLOR_SPECULAR, specular) != AI_SUCCESS) {
                DBG("failed to get specular color for mesh {}", aMesh->mName);
            }

            DBG("creating mesh with {} vertices and {} indices", vertices.size(), indices.size());
            auto  program = WATO_PROGRAM_CACHE["blinnphong"_hs];
            auto* m       = new BlinnPhongMaterial(
                program,
                glm::vec3(diffuse.r, diffuse.g, diffuse.b),
                glm::vec3(specular.r, specular.g, specular.b));

            mp = new MeshPrimitive(std::move(vertices), std::move(indices), m);
        }
    } else {
        DBG("no material in mesh {}", aMesh->mName);
        throw std::runtime_error("no material in mesh");
    }
    return mp;
}

void ModelLoader::processMetaData(const aiNode* aNode, const aiScene* /*aScene*/)
{
    if (!aNode->mMetaData) {
        DBG("node {} metadata is null", aNode->mName);
        return;
    }
    DBG("node {} has {} metadata", aNode->mName, aNode->mMetaData->mNumProperties);
    auto* mdata = aNode->mMetaData;
    for (unsigned int propIdx = 0; propIdx < mdata->mNumProperties; ++propIdx) {
        auto& key = mdata->mKeys[propIdx];
        auto& val = mdata->mValues[propIdx];

        switch (val.mType) {
            case AI_BOOL:
                DBG("{}: bool", key);
                break;
            case AI_INT32:
                DBG("{}: int32", key);
                break;
            case AI_UINT64:
                DBG("{}: uint64", key);
                break;
            case AI_FLOAT:
                DBG("{}: float", key);
                break;
            case AI_DOUBLE:
                DBG("{}: double", key);
                break;
            case AI_AISTRING:
                DBG("{}: aistring", key);
                break;
            case AI_AIVECTOR3D:
                DBG("{}: vector3", key);
                break;
            case AI_AIMETADATA:
                DBG("{}: mdata", key);
                break;
            case AI_INT64:
                DBG("{}: int64", key);
                break;
            case AI_UINT32:
                DBG("{}: uint32", key);
                break;
            case AI_META_MAX:
                DBG("{}: meta max", key);
                break;
            case FORCE_32BIT:
                DBG("{}: force 32bit", key);
                break;
        }
    }
}

ModelLoader::mesh_container ModelLoader::processNode(const aiNode* aNode, const aiScene* aScene)
{
    auto t         = aNode->mTransformation;
    auto transform = glm::identity<glm::mat4>();
    if (!aNode->mTransformation.IsIdentity()) {
        transform = glm::mat4(
            t.a1,
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
        DBG("node {} has transformation {}", aNode->mName, glm::to_string(transform));
    } else {
        DBG("node {} has identity transform", aNode->mName);
    }

    processMetaData(aNode, aScene);
    mesh_container meshes;
    for (unsigned int i = 0; i < aNode->mNumMeshes; ++i) {
        auto* mesh = processMesh(aScene->mMeshes[aNode->mMeshes[i]], aScene);
        meshes.push_back(mesh);
    }
    for (unsigned int i = 0; i < aNode->mNumChildren; i++) {
        auto childMeshes = processNode(aNode->mChildren[i], aScene);
        meshes.insert(meshes.end(), childMeshes.begin(), childMeshes.end());
    }
    return meshes;
}
