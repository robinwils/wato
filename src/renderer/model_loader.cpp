#include <assimp/material.h>
#include <assimp/postprocess.h>  // Post processing flags
#include <assimp/scene.h>        // Output data structure
#include <bx/bx.h>
#include <tinystl/buffer.h>

#include <glm/gtx/string_cast.hpp>
#include <list>
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>

#include "core/sys/log.hpp"
#include "renderer/blinn_phong_material.hpp"
#include "renderer/cache.hpp"
#include "renderer/shader.hpp"

using namespace entt::literals;

constexpr inline glm::vec3 toGLMVec3(const aiVector3D& aVector)
{
    return glm::vec3(aVector.x, aVector.y, aVector.z);
}

constexpr inline glm::mat4 toGLMMat4(const aiMatrix4x4& aMat)
{
    // clang-format off
    return glm::mat4(
        aMat.a1, aMat.b1, aMat.c1, aMat.d1,
        aMat.a2, aMat.b2, aMat.c2, aMat.d2,
        aMat.a3, aMat.b3, aMat.c3, aMat.d3,
        aMat.a4, aMat.b4, aMat.c4, aMat.d4
    );
    // clang-format on
}

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

void ModelLoader::processBones(
    const aiMesh*                            aMesh,
    const aiScene*                           aScene,
    std::vector<PositionNormalUvBoneVertex>& aVertices,
    Skeleton&                                aSkeleton)
{
    std::vector<std::vector<std::pair<float, int>>> boneInfluences(aVertices.size());
    for (unsigned int boneIdx = 0; boneIdx < aMesh->mNumBones; ++boneIdx) {
        const aiBone* bone = aMesh->mBones[boneIdx];

        TRACE("  bone {} with {} vertex weights", bone->mName, bone->mNumWeights);

        if (bone->mNumWeights > 0) {
            std::vector<std::pair<aiVertexWeight, int>> influences;
            float                                       totalWeight = 0.0f;
            for (unsigned int wIdx = 0; wIdx < bone->mNumWeights; ++wIdx) {
                const aiVertexWeight& vertexW = bone->mWeights[wIdx];

        for (unsigned int wIdx = 0; wIdx < bone->mNumWeights; ++wIdx) {
            const aiVertexWeight& vertexW = bone->mWeights[wIdx];

                influences.emplace_back(vertexW, static_cast<int>(mBonesMap[bone->mName.C_Str()]));

            boneInfluences[vertexW.mVertexId].emplace_back(vertexW.mWeight, *skBoneIdx);

            TRACE(
                "    vertex weight {} with {} vertex weights",
                vertexW.mVertexId,
                vertexW.mWeight);
        }
    }

    for (unsigned int i = 0; i < boneInfluences.size(); ++i) {
        std::vector<std::pair<float, int>> influences = boneInfluences[i];
        PositionNormalUvBoneVertex&        vertex     = aVertices[i];

        std::sort(influences.begin(), influences.end(), [](const auto& a, const auto& b) {
            return a.first > b.first;
        });

        float totalWeight = 0.0f;
        for (int j = 0; j < influences.size() && j < 4; ++j) {
            vertex.BoneWeights[j] = influences[j].first;
            vertex.BoneIndices[j] = influences[j].second;
            DBG("keeping normalized vertex weight for bone index {}: {}",
                vertex.BoneIndices[j],
                vertex.BoneWeights[j]);
            totalWeight += vertex.BoneWeights[j];
        }

        if (totalWeight > 0.0f) {
            for (int j = 0; j < 4 && vertex.BoneWeights[j] != -1; ++j) {
                vertex.BoneWeights[j] /= totalWeight;
            }
        }
    }
}

template <typename VL>
ModelLoader::mesh_type
ModelLoader::processMesh(const aiMesh* aMesh, const aiScene* aScene, Skeleton& aSkeleton)
{
    TRACE(
        "mesh {} has {} vertices, {} indices, {} bones",
        aMesh->mName,
        aMesh->mNumVertices,
        aMesh->mNumFaces,
        aMesh->mNumBones);
    std::vector<VL> vertices;
    for (unsigned int i = 0; i < aMesh->mNumVertices; ++i) {
        VL vertex;
        vertex.Position = toGLMVec3(aMesh->mVertices[i]);
        vertex.Normal   = toGLMVec3(aMesh->mNormals[i]);

        if (aMesh->mTextureCoords[0]) {
            vertex.Uv.x = aMesh->mTextureCoords[0][i].x;
            vertex.Uv.y = aMesh->mTextureCoords[0][i].y;
        }
        vertices.push_back(vertex);
    }

    std::vector<typename Primitive<VL>::indice_type> indices;
    for (unsigned int i = 0; i < aMesh->mNumFaces; ++i) {
        auto face = aMesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j) {
            indices.push_back(static_cast<uint16_t>(face.mIndices[j]));
        }
    }

    BlinnPhongMaterial* m       = nullptr;
    bool                skinned = false;
    if (aMesh->mMaterialIndex >= 0) {
        auto        diffusePath  = aiString("texture_diffuse");
        auto        specularPath = aiString("texture_specular");
        const auto* material     = aScene->mMaterials[aMesh->mMaterialIndex];

        auto textures = processMaterialTextures(material, aiTextureType_DIFFUSE, &diffusePath);
        auto specTextures =
            processMaterialTextures(material, aiTextureType_SPECULAR, &specularPath);
        entt::resource<Shader> shader;
        if constexpr (std::is_same_v<VL, PositionNormalUvBoneVertex>) {
            shader  = WATO_PROGRAM_CACHE["blinnphong_skinned"_hs];
            skinned = true;
        } else {
            shader = WATO_PROGRAM_CACHE["blinnphong"_hs];
        }

        TRACE("  {} diffuse and {} specular textures", textures.size(), specTextures.size());
        if (textures.size() > 0 || specTextures.size() > 0) {
            m = new BlinnPhongMaterial(
                shader,
                WATO_TEXTURE_CACHE[textures.front()],
                WATO_TEXTURE_CACHE[specTextures.front()],
                skinned);
        } else {
            // no material textures, get material info via properties
            aiColor3D diffuse;
            if (material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse) != AI_SUCCESS) {
                WARN("failed to get diffuse color for mesh {}", aMesh->mName);
            }
            aiColor3D specular;
            if (material->Get(AI_MATKEY_COLOR_SPECULAR, specular) != AI_SUCCESS) {
                WARN("failed to get specular color for mesh {}", aMesh->mName);
            }

            m = new BlinnPhongMaterial(
                shader,
                glm::vec3(diffuse.r, diffuse.g, diffuse.b),
                glm::vec3(specular.r, specular.g, specular.b));
        }
    } else {
        throw std::runtime_error("no material in mesh");
    }

    if constexpr (std::is_same_v<VL, PositionNormalUvBoneVertex>) {
        if (aMesh->HasBones()) {
            processBones(aMesh, aScene, vertices, aSkeleton);
        }
    }

    return new Primitive(std::move(vertices), std::move(indices), m);
}

void ModelLoader::processMetaData(const aiNode* aNode, const aiScene* /*aScene*/)
{
    if (!aNode->mMetaData) {
        TRACE("  node {} metadata is null", aNode->mName);
        return;
    }
    TRACE("  node {} has {} metadata", aNode->mName, aNode->mMetaData->mNumProperties);
    auto* mdata = aNode->mMetaData;
    for (unsigned int propIdx = 0; propIdx < mdata->mNumProperties; ++propIdx) {
        auto& key = mdata->mKeys[propIdx];
        auto& val = mdata->mValues[propIdx];

        switch (val.mType) {
            case AI_BOOL:
                TRACE("  {}: bool", key);
                break;
            case AI_INT32:
                TRACE("  {}: int32", key);
                break;
            case AI_UINT64:
                TRACE("  {}: uint64", key);
                break;
            case AI_FLOAT:
                TRACE("  {}: float", key);
                break;
            case AI_DOUBLE:
                TRACE("  {}: double", key);
                break;
            case AI_AISTRING:
                TRACE("  {}: aistring", key);
                break;
            case AI_AIVECTOR3D:
                TRACE("  {}: vector3", key);
                break;
            case AI_AIMETADATA:
                TRACE("  {}: mdata", key);
                break;
            case AI_INT64:
                TRACE("  {}: int64", key);
                break;
            case AI_UINT32:
                TRACE("  {}: uint32", key);
                break;
            case AI_META_MAX:
                TRACE("  {}: meta max", key);
                break;
            case FORCE_32BIT:
                TRACE("  {}: force 32bit", key);
                break;
        }
    }
}

ModelLoader::mesh_container
ModelLoader::processNode(const aiNode* aNode, const aiScene* aScene, Skeleton& aSkeleton)
{
    TRACE(
        "node {} with {} meshes, {} children",
        aNode->mName,
        aNode->mNumMeshes,
        aNode->mNumChildren);

    // processMetaData(aNode, aScene);
    mesh_container meshes;
    meshes.reserve(aNode->mNumMeshes);
    for (unsigned int i = 0; i < aNode->mNumMeshes; ++i) {
        std::optional<mesh_type> mesh;
        if (aScene->HasAnimations()) {
            mesh = processMesh<PositionNormalUvBoneVertex>(
                aScene->mMeshes[aNode->mMeshes[i]],
                aScene,
                aSkeleton);
        } else {
            mesh = processMesh<PositionNormalUvVertex>(
                aScene->mMeshes[aNode->mMeshes[i]],
                aScene,
                aSkeleton);
        }
        assert(mesh.has_value());
        meshes.push_back(*mesh);
    }
    for (unsigned int i = 0; i < aNode->mNumChildren; i++) {
        auto childMeshes = processNode(aNode->mChildren[i], aScene, aSkeleton);
        meshes.insert(
            meshes.end(),
            std::make_move_iterator(childMeshes.begin()),
            std::make_move_iterator(childMeshes.end()));
    }
    return meshes;
}

Animation ModelLoader::processAnimation(const aiAnimation* aAnimation)
{
    Animation::node_animation_map nodeAnimations;
    for (unsigned int i = 0; i < aAnimation->mNumChannels; ++i) {
        const aiNodeAnim* channel = aAnimation->mChannels[i];
        NodeAnimation     nodeAnimation;
        TRACE(
            "  node anim {} with {} position keys, {} rotation keys, {} scaling keys",
            channel->mNodeName,
            channel->mNumPositionKeys,
            channel->mNumRotationKeys,
            channel->mNumScalingKeys);

        nodeAnimation.Name = channel->mNodeName.C_Str();
        for (unsigned int posIdx = 0; posIdx < channel->mNumPositionKeys; ++posIdx) {
            const aiVectorKey& posKey = channel->mPositionKeys[posIdx];
            nodeAnimation.Positions.emplace_back(toGLMVec3(posKey.mValue), posKey.mTime);
        }
        for (unsigned int rotIdx = 0; rotIdx < channel->mNumRotationKeys; ++rotIdx) {
            const aiQuatKey& rotationKey = channel->mRotationKeys[rotIdx];
            nodeAnimation.Rotations.emplace_back(
                glm::quat(
                    rotationKey.mValue.x,
                    rotationKey.mValue.y,
                    rotationKey.mValue.z,
                    rotationKey.mValue.w),
                rotationKey.mTime);
        }
        for (unsigned int scKey = 0; scKey < channel->mNumScalingKeys; ++scKey) {
            const aiVectorKey& scalingKey = channel->mScalingKeys[scKey];
            nodeAnimation.Scales.emplace_back(toGLMVec3(scalingKey.mValue), scalingKey.mTime);
        }
        nodeAnimations[channel->mNodeName.C_Str()] = std::move(nodeAnimation);
    }
    return Animation(
        aAnimation->mName.C_Str(),
        aAnimation->mDuration,
        aAnimation->mTicksPerSecond,
        std::move(nodeAnimations));
}

ModelLoader::animation_map ModelLoader::processAnimations(const aiScene* aScene)
{
    ModelLoader::animation_map animations;
    for (unsigned int animIdx = 0; animIdx < aScene->mNumAnimations; ++animIdx) {
        const aiAnimation* anim = aScene->mAnimations[animIdx];
        TRACE(
            "animation {} with duration {}, {} ticks/s",
            anim->mName,
            anim->mDuration,
            anim->mTicksPerSecond);

        animations.insert_or_assign(anim->mName.C_Str(), processAnimation(anim));
    }

    return animations;
}

std::size_t ModelLoader::buildSkeleton(
    const aiNode*  aNode,
    const aiScene* aScene,
    Skeleton&      aSkeleton,
    std::size_t    aIndent)
{
    std::size_t currentBoneIdx = aSkeleton.Bones.size();
    std::string indent(aIndent, ' ');

    TRACE("{}building node {} with {} children", indent, aNode->mName, aNode->mNumChildren);

    aSkeleton.Bones.emplace_back(aNode->mName.C_Str());
    mBonesMap[aNode->mName.C_Str()] = currentBoneIdx;

    for (unsigned int meshIdx = 0; meshIdx < aNode->mNumMeshes; ++meshIdx) {
        const aiMesh* mesh = aScene->mMeshes[aNode->mMeshes[meshIdx]];

        for (unsigned int boneIdx = 0; boneIdx < mesh->mNumBones; ++boneIdx) {
            const aiBone* bone = mesh->mBones[boneIdx];

            auto it = mBonesMap.find(bone->mName.C_Str());
            if (it == mBonesMap.end()) {
                TRACE("{}adding bone {} to skeleton", indent, bone->mName);
                std::size_t currentIdx = aSkeleton.Bones.size();
                mBonesMap.emplace(bone->mName.C_Str(), currentIdx);
                aSkeleton.Bones.emplace_back(bone->mName.C_Str(), toGLMMat4(bone->mOffsetMatrix));
            } else {
                TRACE("{}patching bone {} in skeleton", indent, bone->mName);
                aSkeleton.Bones[it->second].Offset = toGLMMat4(bone->mOffsetMatrix);
            }
        }
    }

    for (unsigned int childIdx = 0; childIdx < aNode->mNumChildren; childIdx++) {
        std::size_t childNodeIndex =
            buildSkeleton(aNode->mChildren[childIdx], aScene, aSkeleton, aIndent + 2);
        aSkeleton.Bones[currentBoneIdx].Children.push_back(childNodeIndex);
    }
    return currentBoneIdx;
}
