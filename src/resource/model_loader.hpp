#pragma once

#include <assimp/postprocess.h>  // Post processing flags
#include <assimp/scene.h>        // Output data structure

#include <assimp/Importer.hpp>  // C++ importer interface
#include <entt/core/hashed_string.hpp>
#include <optional>

#include "registry/registry.hpp"
#include "renderer/animation.hpp"
#include "renderer/model.hpp"
#include "renderer/skeleton.hpp"
#include "renderer/vertex_layout.hpp"

template <>
struct fmt::formatter<aiString> : fmt::formatter<std::string> {
    auto format(aiString aObj, format_context& aCtx) const -> decltype(aCtx.out())
    {
        return fmt::format_to(aCtx.out(), "{}", aObj.C_Str());
    }
};

template <>
struct fmt::formatter<aiMatrix4x4> : fmt::formatter<std::string> {
    auto format(aiMatrix4x4 aObj, format_context& aCtx) const -> decltype(aCtx.out())
    {
        // clang-format off
        return fmt::format_to(
            aCtx.out(),
            "aiMatrix4x4(({}, {}, {}, {}), ({}, {}, {}, {}), ({}, {}, {}, {}), ({}, {}, {}, {}))",
            aObj.a1,aObj.a2,aObj.a3,aObj.a4,
            aObj.b1,aObj.b2,aObj.b3,aObj.b4,
            aObj.c1,aObj.c2,aObj.c3,aObj.c4,
            aObj.d1,aObj.d2,aObj.d3,aObj.d4);
        // clang-format on
    }
};

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

constexpr inline glm::quat toGLMQuat(const aiQuaternion& aQuat)
{
    return glm::quat(aQuat.w, aQuat.x, aQuat.y, aQuat.z);
}

class ModelLoader final
{
   public:
    using mesh_type      = Model::mesh_type;
    using mesh_container = Model::mesh_container;
    using bone_index_map = std::unordered_map<std::string, std::optional<std::size_t>>;
    using animation_map  = Model::animation_map;
    using result_type    = std::shared_ptr<Model>;

    result_type operator()(Registry& aRegistry, const char* aName, unsigned int aPostProcessFlags);
    result_type operator()(mesh_type aPrimitive)
    {
        mesh_container meshes;
        meshes.push_back(std::move(aPrimitive));
        return std::make_shared<Model>(std::move(meshes), animation_map{});
    }

   private:
    std::vector<entt::hashed_string> processMaterialTextures(
        const aiMaterial* aMaterial,
        aiTextureType     aType,
        aiString*         aPath,
        Registry&         aRegistry);

    mesh_container processNode(
        const aiNode*  aNode,
        const aiScene* aScene,
        Skeleton&      aSkeleton,
        Registry&      aRegistry);

    template <typename VL>
    mesh_type processMesh(
        const aiMesh*  aMesh,
        const aiScene* aScene,
        Skeleton&      aSkeleton,
        Registry&      aRegistry);

    void          processMetaData(const aiNode* aNode, const aiScene* /*aScene*/);
    animation_map processAnimations(const aiScene* aScene);
    Animation     processAnimation(const aiAnimation* aAnimation);
    void          processBones(
                 const aiMesh*                            aMesh,
                 std::vector<PositionNormalUvBoneVertex>& aVertices,
                 Skeleton&                                aSkeleton);

    void        populateBoneNames(const aiNode* aNode, const aiScene* aScene);
    std::size_t buildSkeleton(
        const aiNode*  aNode,
        const aiScene* aScene,
        Skeleton&      aSkeleton,
        std::size_t    aIndent);

    bone_index_map mBonesMap;
};
