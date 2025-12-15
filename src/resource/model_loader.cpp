#include "resource/model_loader.hpp"

#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <bx/bx.h>

#include <glm/gtx/string_cast.hpp>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "renderer/blinn_phong_material.hpp"
#include "renderer/shader.hpp"
#include "resource/asset.hpp"
#include "resource/cache.hpp"

using namespace entt::literals;

ModelLoader::result_type
ModelLoader::operator()(Registry& aRegistry, const char* aName, unsigned int aPostProcessFlags)
{
    Assimp::Importer importer;

    std::string assetPath = FindAsset(aName);
    if (assetPath == "") {
        throw std::runtime_error(fmt::format("cannot find asset {}", aName));
    }

    const aiScene* scene = importer.ReadFile(assetPath, aPostProcessFlags);

    // If the import failed, report it
    if (nullptr == scene) {
        throw std::runtime_error(
            fmt::format("could not load {}: {}", assetPath, importer.GetErrorString()));
    }
    spdlog::debug("model {} has:", aName);
    spdlog::debug("  {} meshes", scene->mNumMeshes);
    spdlog::debug("  {} embedded textures", scene->mNumTextures);
    spdlog::debug("  {} materials", scene->mNumMaterials);
    spdlog::debug("  {} animations", scene->mNumAnimations);

    Skeleton skeleton;
    if (scene->HasAnimations()) {
        populateBoneNames(scene->mRootNode, scene);
        spdlog::trace("got bone names: {}", mBonesMap);
        buildSkeleton(scene->mRootNode, scene, skeleton, 0);
        spdlog::debug("skeleton with {} bones built for model {}", skeleton.Bones.size(), aName);
        if (!mBonesMap.empty()) {
            for (std::size_t i = 0; i < skeleton.Bones.size(); ++i) {
                spdlog::trace("Bone[{}]: {}", i, skeleton.Bones[i]);
            }
        } else {
            spdlog::warn("got animations but no bones");
        }
    }
    auto          meshes = processNode(scene->mRootNode, scene, skeleton, aRegistry);
    animation_map animations;
    if (scene->HasAnimations()) {
        animations = processAnimations(scene);
    }

    return std::make_shared<Model>(
        std::move(meshes),
        std::move(animations),
        std::move(skeleton),
        toGLMMat4(scene->mRootNode->mTransformation.Inverse()));
}

std::vector<entt::hashed_string> ModelLoader::processMaterialTextures(
    const aiMaterial* aMaterial,
    aiTextureType     aType,
    aiString*         aPath,
    Registry&         aRegistry)
{
    std::vector<entt::hashed_string> textures;
    for (unsigned int i = 0; i < aMaterial->GetTextureCount(aType); ++i) {
        aiReturn res = aMaterial->GetTexture(aType, i, aPath);
        if (AI_SUCCESS != res) {
            throw std::runtime_error("could not get texture");
        }

        const auto& diffuse =
            LoadResource(aRegistry.ctx().get<TextureCache>(), aPath->C_Str(), aPath->C_Str());

        textures.emplace_back(aPath->C_Str());
    }

    return textures;
}

void ModelLoader::processBones(
    const aiMesh*                            aMesh,
    std::vector<PositionNormalUvBoneVertex>& aVertices,
    Skeleton&                                aSkeleton)
{
    std::vector<std::vector<std::pair<float, int>>> boneInfluences(aVertices.size());
    for (unsigned int boneIdx = 0; boneIdx < aMesh->mNumBones; ++boneIdx) {
        const aiBone*                     bone      = aMesh->mBones[boneIdx];
        const std::optional<std::size_t>& skBoneIdx = mBonesMap[bone->mName.C_Str()];

        spdlog::trace(
            "  bone {} (idx: {}) with {} vertex weights",
            bone->mName,
            skBoneIdx,
            bone->mNumWeights);

        if (bone->mNumWeights == 0) {
            continue;
        }

        for (unsigned int wIdx = 0; wIdx < bone->mNumWeights; ++wIdx) {
            const aiVertexWeight& vertexW = bone->mWeights[wIdx];

            BX_ASSERT(
                vertexW.mVertexId < aVertices.size(),
                "vertex ID bigger than vertices parsed");

            spdlog::trace(
                "    vertex weight {} with {} vertex weights",
                vertexW.mVertexId,
                vertexW.mWeight);

            boneInfluences[vertexW.mVertexId].emplace_back(vertexW.mWeight, *skBoneIdx);
            if (skBoneIdx && !aSkeleton.Bones[*skBoneIdx].Offset) {
                aSkeleton.Bones[*skBoneIdx].Offset.emplace(toGLMMat4(bone->mOffsetMatrix));
            }

            spdlog::trace(
                "    vertex weight {} with {} vertex weights",
                vertexW.mVertexId,
                vertexW.mWeight);
        }
    }

    for (unsigned int i = 0; i < boneInfluences.size(); ++i) {
        std::vector<std::pair<float, int>> influences = boneInfluences[i];
        PositionNormalUvBoneVertex&        vertex     = aVertices[i];

        std::sort(influences.begin(), influences.end(), [](const auto& aX, const auto& aY) {
            return aX.first > aY.first;
        });

        float totalWeight = 0.0f;
        for (size_t j = 0; j < influences.size() && j < 4; ++j) {
            vertex.BoneWeights[j] = influences[j].first;
            vertex.BoneIndices[j] = static_cast<float>(influences[j].second);
            spdlog::trace(
                "keeping normalized vertex weight for bone index {}: {}",
                vertex.BoneIndices[j],
                vertex.BoneWeights[j]);
            totalWeight += vertex.BoneWeights[j];
        }

        if (totalWeight > 0.0f) {
            for (size_t j = 0; j < 4 && vertex.BoneWeights[j] != -1; ++j) {
                vertex.BoneWeights[j] /= totalWeight;
            }
        }
    }
}

template <typename VL>
ModelLoader::mesh_type ModelLoader::processMesh(
    const aiMesh*  aMesh,
    const aiScene* aScene,
    Skeleton&      aSkeleton,
    Registry&      aRegistry)
{
    spdlog::trace(
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

    std::unique_ptr<BlinnPhongMaterial> m;
    bool                                skinned = false;
    if (aMesh->mMaterialIndex >= 0) {
        auto        diffusePath  = aiString("texture_diffuse");
        auto        specularPath = aiString("texture_specular");
        const auto* material     = aScene->mMaterials[aMesh->mMaterialIndex];

        auto textures =
            processMaterialTextures(material, aiTextureType_DIFFUSE, &diffusePath, aRegistry);
        auto specTextures =
            processMaterialTextures(material, aiTextureType_SPECULAR, &specularPath, aRegistry);
        entt::resource<Shader> shader;
        if constexpr (std::is_same_v<VL, PositionNormalUvBoneVertex>) {
            shader  = aRegistry.ctx().get<ShaderCache>()["blinnphong_skinned"_hs];
            skinned = true;
        } else {
            shader = aRegistry.ctx().get<ShaderCache>()["blinnphong"_hs];
        }

        if (!bgfx::isValid(shader->Program())) {
            throw std::runtime_error(
                fmt::format("could not load blinnphong shader {}", skinned ? "skinned" : ""));
        }

        spdlog::trace(
            "  {} diffuse and {} specular textures",
            textures.size(),
            specTextures.size());
        m = std::make_unique<BlinnPhongMaterial>(shader);

        if (textures.size() > 0) {
            m->SetDiffuseTexture(aRegistry.ctx().get<TextureCache>()[textures.front()]);
        } else {
            // no material textures, get material info via properties
            aiColor3D diffuse;
            if (material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse) != AI_SUCCESS) {
                spdlog::warn("failed to get diffuse color for mesh {}", aMesh->mName);
            }
            m->SetDiffuse(glm::vec3(diffuse.r, diffuse.g, diffuse.b));
        }

        if (specTextures.size() > 0) {
            m->SetSpecularTexture(aRegistry.ctx().get<TextureCache>()[specTextures.front()]);
        } else {
            aiColor3D specular;
            if (material->Get(AI_MATKEY_COLOR_SPECULAR, specular) != AI_SUCCESS) {
                spdlog::warn("failed to get specular color for mesh {}", aMesh->mName);
            }
            m->SetSpecular(glm::vec3(specular.r, specular.g, specular.b));
        }
    } else {
        throw std::runtime_error("no material in mesh");
    }

    if constexpr (std::is_same_v<VL, PositionNormalUvBoneVertex>) {
        if (aMesh->HasBones()) {
            processBones(aMesh, vertices, aSkeleton);
        }
    }

    return std::make_unique<Primitive<VL>>(std::move(vertices), std::move(indices), std::move(m));
}

void ModelLoader::processMetaData(const aiNode* aNode, const aiScene* /*aScene*/)
{
    if (!aNode->mMetaData) {
        spdlog::trace("  node {} metadata is null", aNode->mName);
        return;
    }
    spdlog::trace("  node {} has {} metadata", aNode->mName, aNode->mMetaData->mNumProperties);
    auto* mdata = aNode->mMetaData;
    for (unsigned int propIdx = 0; propIdx < mdata->mNumProperties; ++propIdx) {
        auto& key = mdata->mKeys[propIdx];
        auto& val = mdata->mValues[propIdx];

        switch (val.mType) {
            case AI_BOOL:
                spdlog::trace("  {}: bool", key);
                break;
            case AI_INT32:
                spdlog::trace("  {}: int32", key);
                break;
            case AI_UINT64:
                spdlog::trace("  {}: uint64", key);
                break;
            case AI_FLOAT:
                spdlog::trace("  {}: float", key);
                break;
            case AI_DOUBLE:
                spdlog::trace("  {}: double", key);
                break;
            case AI_AISTRING:
                spdlog::trace("  {}: aistring", key);
                break;
            case AI_AIVECTOR3D:
                spdlog::trace("  {}: vector3", key);
                break;
            case AI_AIMETADATA:
                spdlog::trace("  {}: mdata", key);
                break;
            case AI_INT64:
                spdlog::trace("  {}: int64", key);
                break;
            case AI_UINT32:
                spdlog::trace("  {}: uint32", key);
                break;
            case AI_META_MAX:
                spdlog::trace("  {}: meta max", key);
                break;
            case FORCE_32BIT:
                spdlog::trace("  {}: force 32bit", key);
                break;
        }
    }
}

ModelLoader::mesh_container ModelLoader::processNode(
    const aiNode*  aNode,
    const aiScene* aScene,
    Skeleton&      aSkeleton,
    Registry&      aRegistry)
{
    spdlog::trace(
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
                aSkeleton,
                aRegistry);
        } else {
            mesh = processMesh<PositionNormalUvVertex>(
                aScene->mMeshes[aNode->mMeshes[i]],
                aScene,
                aSkeleton,
                aRegistry);
        }
        assert(mesh.has_value());
        meshes.push_back(std::move(*mesh));
    }
    for (unsigned int i = 0; i < aNode->mNumChildren; i++) {
        auto childMeshes = processNode(aNode->mChildren[i], aScene, aSkeleton, aRegistry);
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
        spdlog::trace(
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
            nodeAnimation.Rotations.emplace_back(toGLMQuat(rotationKey.mValue), rotationKey.mTime);
        }
        for (unsigned int scKey = 0; scKey < channel->mNumScalingKeys; ++scKey) {
            const aiVectorKey& scalingKey = channel->mScalingKeys[scKey];
            nodeAnimation.Scales.emplace_back(toGLMVec3(scalingKey.mValue), scalingKey.mTime);
        }
        nodeAnimations[channel->mNodeName.C_Str()] = std::move(nodeAnimation);
    }
    return Animation(
        aAnimation->mName.C_Str(),
        aAnimation->mDuration != 0 ? aAnimation->mDuration : 25.0,
        aAnimation->mTicksPerSecond,
        std::move(nodeAnimations));
}

ModelLoader::animation_map ModelLoader::processAnimations(const aiScene* aScene)
{
    ModelLoader::animation_map animations;
    for (unsigned int animIdx = 0; animIdx < aScene->mNumAnimations; ++animIdx) {
        const aiAnimation* anim = aScene->mAnimations[animIdx];
        spdlog::trace(
            "animation {} with duration {}, {} ticks/s",
            anim->mName,
            anim->mDuration,
            anim->mTicksPerSecond);

        animations.insert_or_assign(anim->mName.C_Str(), processAnimation(anim));
    }

    return animations;
}

void ModelLoader::populateBoneNames(const aiNode* aNode, const aiScene* aScene)
{
    for (unsigned int meshIdx = 0; meshIdx < aNode->mNumMeshes; ++meshIdx) {
        const aiMesh* mesh = aScene->mMeshes[aNode->mMeshes[meshIdx]];

        for (unsigned int boneIdx = 0; boneIdx < mesh->mNumBones; ++boneIdx) {
            const aiBone* bone = mesh->mBones[boneIdx];

            if (!mBonesMap.contains(bone->mName.C_Str())) {
                mBonesMap.emplace(bone->mName.C_Str(), std::nullopt);
            }
        }
    }
    for (unsigned int childIdx = 0; childIdx < aNode->mNumChildren; childIdx++) {
        populateBoneNames(aNode->mChildren[childIdx], aScene);
    }
}
std::size_t ModelLoader::buildSkeleton(
    const aiNode*  aNode,
    const aiScene* aScene,
    Skeleton&      aSkeleton,
    std::size_t    aIndent)
{
    std::size_t currentBoneIdx = aSkeleton.Bones.size();
    std::string indent(aIndent, ' ');
    const bool  isBone = mBonesMap.contains(aNode->mName.C_Str());

    spdlog::trace(
        "{}building node {} with {} children, {} meshes, is bone ? {}",
        indent,
        aNode->mName,
        aNode->mNumChildren,
        aNode->mNumMeshes,
        isBone);

    aSkeleton.Bones.emplace_back(aNode->mName.C_Str(), toGLMMat4(aNode->mTransformation));
    mBonesMap[aNode->mName.C_Str()] = currentBoneIdx;

    for (unsigned int childIdx = 0; childIdx < aNode->mNumChildren; childIdx++) {
        std::size_t childNodeIndex =
            buildSkeleton(aNode->mChildren[childIdx], aScene, aSkeleton, aIndent + 2);
        aSkeleton.Bones[currentBoneIdx].Children.push_back(childNodeIndex);
    }
    return currentBoneIdx;
}
